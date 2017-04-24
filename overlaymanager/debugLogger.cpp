#include "debugLogger.h"

#include <QApplication>
#include <QDesktopServices>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QCryptographicHash>
#ifdef Q_OS_ANDROID
#include <QtAndroidExtras/QAndroidJniObject>
#else
#include <QHostInfo>
#endif
#include <QThread>
#include <QUrl>
#include <QStandardPaths>
#include <QMessageBox>
#include <QDir>
#include <QFile>
#include <QTime>
#include <QDebug>

// Okay, I've writen this in C, but it is a singleton with virually no API.

#if defined(Q_OS_WIN) || defined(Q_OS_ANDROID)
#include <QFile>
#include <QTextStream>

static QString                debugOutputFileName;
static QString                sendFilename;
static QString                debugID;
static QFile                  debugCaptureFile;
static QFile                 *debugSendFile;
static QTextStream            debugOutStream;
static bool                   debugOpened = false;
static int                    numRepeats  = 0;
static QString                lastOutputLine;
static bool                   debugFilerInstalled = false;
static QtMessageHandler       oldDebugFiler;
static bool                   uploadInProgress = false;
static QNetworkAccessManager *manager;
static QNetworkReply         *reply;
static bool                   debugDataIsAvailableToSend = false;


void closeAndFlushDebugFiler(void)
{
    if( debugFilerInstalled )
    {
        qDebug() << "Debug run stop:" << QDate::currentDate().toString() << QTime::currentTime().toString();

        qInstallMessageHandler( oldDebugFiler );
    }
    debugFilerInstalled = false;

    if( debugOpened )
    {
        if( numRepeats>0 )
        {
            debugOutStream << QString(" [%1 repeats]").arg(numRepeats) << endl;
            lastOutputLine = "";
            numRepeats     = 0;
        }

        qDebug() << "Flush redirected debug stream";

        debugOutStream.flush();
        debugCaptureFile.close();

        debugOpened = false;

        // Dump it all to the debug stream
        if( QFile::exists(debugOutputFileName) )
        {
            QFile debugFile(debugOutputFileName);

            if( ! debugFile.open(QIODevice::ReadOnly) )
            {
                qDebug() << "Failed to open debug file to send to log.";
            }
            else
            {
                char    readBuf[512];
                int     lineSize;
                QString debugStr;

                while( ! debugFile.atEnd() )
                {
                    lineSize = debugFile.readLine(readBuf, sizeof(readBuf) );
                    if( lineSize > 0 )
                    {
                        if( readBuf[lineSize-1] == '\n' ) readBuf[lineSize-1] = (char)0;
                        debugStr += QString::fromLocal8Bit(readBuf);
                    }
                }

                qDebug() << debugStr;
            }

            if( QFile::exists(sendFilename) && ! QFile::remove(sendFilename) )
            {
                qDebug() << "Failed to delete old send debug file.";
            }
        }
    }
}


static QString getDeviceID(void)
{
#ifdef Q_OS_ANDROID
    // Get AndroidId to act as a device identifier

    QAndroidJniObject andID      = QAndroidJniObject::fromString("android_id");
    QAndroidJniObject activity   = QAndroidJniObject::callStaticObjectMethod(
                                             "org/qtproject/qt5/android/QtNative",
                                             "activity", "()Landroid/app/Activity;");
    QAndroidJniObject appContext = activity.callObjectMethod(
                                             "getApplicationContext",
                                             "()Landroid/content/Context;");
    QAndroidJniObject resolver   = appContext.callObjectMethod(
                                             "getContentResolver",
                                             "()Landroid/content/ContentResolver;");
    QAndroidJniObject result     = QAndroidJniObject::callStaticObjectMethod(
                                             "android/provider/Settings$Secure",
                                             "getString",
                                             "(Landroid/content/ContentResolver;Ljava/lang/String;)Ljava/lang/String;",
                                             resolver.object<jobject>(),
                                             andID.object<jstring>()    );

    QString  androidID = result.toString();

    // Munge it so it's not made public when we send it on...

    QByteArray source = QString("Extracted android_id: %1.").arg(androidID).toUtf8();

    QCryptographicHash hash(QCryptographicHash::Sha256);
    hash.addData(source);

    QByteArray ident = hash.result().right(4).toHex();

    return QString(ident);
#else
    return QHostInfo::localHostName();
#endif
}



static void qDebugFiler(QtMsgType type, const QMessageLogContext &context, const QString &message)
{
    // Ensure we have a file to write to
    if( ! debugOpened )
    {
        debugCaptureFile.setFileName(debugOutputFileName);

        if( ! debugCaptureFile.open(QIODevice::Unbuffered|QIODevice::WriteOnly|
                                    QIODevice::Text      |QIODevice::Truncate  ) )
        {
            // NB .data() returns a NULL terminates string
            fprintf(stderr,"## %s",message.toLocal8Bit().data());
            return;
        }

        debugOutStream.setDevice(&debugCaptureFile);

        debugOpened = true;
    }

    // Output as appropriate

    QString outputLine;

    switch( type )
    {
    case QtDebugMsg:    outputLine = "D: ";  break;
    case QtWarningMsg:  outputLine = "W: ";  break;
    case QtCriticalMsg: outputLine = "C: ";  break;
    case QtFatalMsg:    outputLine = "F: ";  break;
    default:            outputLine = "?: ";
    }

    outputLine = outputLine + QTime::currentTime().toString() + " ";
    outputLine = outputLine + context.function + QString(" ln %1 ").arg(context.line) + message;

    if( outputLine == lastOutputLine )
    {
        numRepeats ++;
    }
    else
    {
        if( numRepeats>0 )
        {
            debugOutStream << QString(" [%1 repeats]").arg(numRepeats) << endl;
            numRepeats = 0;
        }

        debugOutStream << outputLine << endl;

        lastOutputLine = outputLine;
    }

    if( type == QtFatalMsg )
    {
        closeAndFlushDebugFiler();
        abort();
    }
}

#endif

#define FTP_SERVER   "ftp.byethost5.com"
#define FTP_USERNAME "b5_15230050"
#define FTP_PASSWORD "bananaman57"

void initDebugLogger(void)
{
//    Not writable on Android: QStandardPaths::standardLocations(QStandardPaths::DocumentsLocation)
//    Not writable on Android: QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation)
//    Home is writable: QStandardPaths::writableLocation(QStandardPaths::HomeLocation);
#if defined(Q_OS_ANDROID) || defined(Q_OS_WIN)
    debugDataIsAvailableToSend = false;
    uploadInProgress           = false;
    manager                    = new QNetworkAccessManager;

    debugOutputFileName = QStandardPaths::writableLocation(QStandardPaths::HomeLocation)+
                          QDir::separator()+"betaDebug.txt";
    sendFilename        = QStandardPaths::writableLocation(QStandardPaths::HomeLocation)+
                          QDir::separator()+"sendDebug.txt";

    debugID = getDeviceID();

    // Check for existing debug file
    if( QFile::exists(debugOutputFileName) )
    {
        qDebug() << "Previous debug data file exists!";

        QFile   fileActions(sendFilename);

        if( fileActions.exists() && ! fileActions.remove() )
        {
            qDebug() << "Failed to delete old send debug file.";
        }
        fileActions.setFileName(debugOutputFileName);
        if( ! fileActions.rename(sendFilename) )
        {
            qDebug() << "Failed to rename last debug to send debug file.";
        }

        debugDataIsAvailableToSend = true;
    }

#if defined(Q_OS_WIN) || defined(Q_OS_ANDROID)

    oldDebugFiler       = qInstallMessageHandler( qDebugFiler );
    debugFilerInstalled = true;

    qDebug() << "Debug run start:" << QDate::currentDate().toString() << QTime::currentTime().toString();

#endif

#if defined( Q_OS_ANDROID ) || defined( Q_OS_LINUX )

    // Install crash handler (gcc only)

#endif
#endif
}

bool debugAvailableToSend(void)
{
#if defined(Q_OS_ANDROID) || defined(Q_OS_WIN)
    return debugDataIsAvailableToSend;
#else
    return false;
#endif
}


QNetworkAccessManager *sendDebugToServer(void)
{
#if defined(Q_OS_ANDROID) || defined(Q_OS_WIN)
    debugSendFile = new QFile(sendFilename);
    if( ! debugSendFile->open(QIODevice::ReadOnly) )
    {
        qDebug() << "Failed to open debug file to send to Cosneta.";
    }
    else
    {
        QString urlString = QString("ftp://%1/htdocs/%2_betaDebug.txt")
                                    .arg(FTP_SERVER).arg(debugID);
        QUrl    url(urlString);

        url.setUserName(FTP_USERNAME);
        url.setPassword(FTP_PASSWORD);

//        qDebug() << "put the url:" << url;

        reply = manager->put(QNetworkRequest(url),debugSendFile);

        // TODO: trap the response (may make this a class to use connect & SLOTs
        qDebug() << "Started the send and forget process...";
    }

    return manager;
#else
    return NULL;
#endif
}

void sendDebugToServerComplete(void)
{
#if defined(Q_OS_ANDROID) || defined(Q_OS_WIN)
    debugSendFile->close();

    debugDataIsAvailableToSend = false;

    manager->deleteLater();
#endif
}


/* ************** */
/* Crash handling */
/* ************** */
#if defined(Q_OS_LINUX) || defined(Q_OS_ANDROID)
#define NUM_SIGNALS 6
#include "signal.h"
static const int signal_list[NUM_SIGNALS] = { SIGABRT, SIGILL, SIGTRAP, SIGBUS, SIGFPE, SIGSEGV };
struct sigaction sa_old[NUM_SIGNALS];

void crash_handler(int code, siginfo_t *si, void *sc)
{
    // Check for abort
    if( code == SIGABRT )
    {
        printf("Caught abort.\n");
        abort();
    }

    printf("Caught signal %d.\n", code);
#if 0
    // In case this is a signal used in Java, do the normal handling first
    for(int i=0; i<NUM_SIGNALS; i++)
    {
        if(code == signal_list[i])
        {
            if( sa_old[i].sa_sigaction != NULL )
            {
                sa_old[i].sa_sigaction(code,si,sc);
            }
            break;
        }
    }

    abort();
#endif
}


void init_crash_handler(void)
{
    // An alternate stack in case the original crash was caused ba the main stack being filled
    stack_t stack;


    // Install signal handler
    struct sigaction sa;

    bzero(&sa,sizeof(sa));
    sigemptyset(&sa.sa_mask);
    sa.sa_sigaction = crash_handler;
    sa.sa_flags  = SA_SIGINFO|SA_ONSTACK;

    for(int sig_idx=0; sig_idx<NUM_SIGNALS; sig_idx++)
    {
        if( sigaction(signal_list[sig_idx], &sa, &(sa_old[sig_idx])) != 0 )
        {
            qDebug() << "Failed to register signal handler index" << sig_idx
                     << "(" << signal_list[sig_idx] << ") using sigaction()";
            return;
        }
    }
}
#endif

