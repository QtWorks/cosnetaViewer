#include <QApplication>
#include <QMessageBox>
#include <QDebug>
#include <QCoreApplication>
// For file based updater
#include <QFile>
#include <QDir>
#include <QRegularExpression>
#include <QMessageBox>

#include "main.h"
#include "updater.h"

#ifdef Q_OS_WIN
#include "io.h"
#endif

updater::updater(QWidget *parent) : QWidget(parent)
{
    // Create the dialogue
    QVBoxLayout *updateLayout = new QVBoxLayout;

    QLabel  *question = new QLabel(tr("Updates available. Do you wish to continue?"));

    QHBoxLayout *continueQuestionLayout = new QHBoxLayout;

    yesDoUpdate = new QPushButton(tr("&Yes, download updates"),this);
    connect(yesDoUpdate, SIGNAL(clicked()), this, SLOT(startSoftwareUpdate()));

    noUpdate = new QPushButton(tr("&No, ignore updates for now"),this);
    connect(noUpdate, SIGNAL(clicked()), this, SLOT(abortUpdate()));

    continueQuestionLayout->addWidget(yesDoUpdate);
    continueQuestionLayout->addWidget(noUpdate);

    QGridLayout *downloadProgressLayout = new QGridLayout;

    QLabel *dloadProgressLabel  = new QLabel(tr("Download "));
    downloadProgress            = new QProgressBar;

    downloadProgress->setRange(0,100);
    downloadProgress->setValue(0);

    downloadProgressLayout->addWidget(dloadProgressLabel, 0,0);
    downloadProgressLayout->addWidget(downloadProgress,   0,1);

    updateLayout->addWidget(question);
    updateLayout->addStretch(10);
    updateLayout->addItem(continueQuestionLayout);
    updateLayout->addStretch(10);
    updateLayout->addItem(downloadProgressLayout);

    setLayout(updateLayout);

    updateTimeStamp  = 0;
    updateFileLength = 0;

    downloadManager    = NULL;
    downloadInProgress = false;

    downloadHash = new QCryptographicHash(QCryptographicHash::Sha256);

    downloadData.clear();

    hide(); // Yes, but just to be sure
}

void updater::startUpdateDialogueIfValid(QString metaData)
{
    qDebug() << "Have downloaded latest metaData:" << QString(metaData);

    QStringList metaWords = metaData.split(QRegularExpression("\\s+"));

    // [OS TimeStamp Length Hash]

    if( metaWords[0]     != QString("Win")   ||
        metaWords[4]     != QString("Linux") ||
        metaWords[8]     != QString("OSX")   ||
        metaWords.size() != 12                  )
    {
        balloonMessage(tr("WARNING: Bad data returned from update server!"));
        return;
    }

    if( isVisible() )
    {
        balloonMessage(tr("Do not re-show update dialogue."));
        return;
    }

    // Extract the latest server versions available

#if defined(Q_OS_WIN)

    updateTimeStamp  = metaWords[1].toInt();
    updateFileLength = metaWords[2].toInt();
    updateShaString  = metaWords[3];

#elif defined(Q_OS_LINUX)

    updateTimeStamp  = metaWords[5].toInt();
    updateFileLength = metaWords[6].toInt();
    updateShaString  = metaWords[7];

#elif defined(Q_OS_MAC)

    updateTimeStamp  = metaWords[9].toInt();
    updateFileLength = metaWords[10].toInt();
    updateShaString  = metaWords[11];

#endif

    // Make sure the buttons are active
    yesDoUpdate->setEnabled(true);
    noUpdate->setEnabled(true);

    // Read the current version (will be in license, when that's been done correctly)

    int currentVersionTimeStamp = 0;

    QString versionFileName = QApplication::applicationDirPath() + QString(VERSION_FILE);
    if( QFile::exists(versionFileName) )
    {
        QFile readFile(versionFileName);

        if( readFile.open(QFile::ReadOnly) )
        {
            QByteArray path = QApplication::applicationDirPath().toLatin1();

            readFile.read((char *)&currentVersionTimeStamp, sizeof(currentVersionTimeStamp));
        }
    }

    // Update required?
    if( updateTimeStamp <= currentVersionTimeStamp )
    {
        balloonMessage(tr("No software updates available."));
        return;
    }

    // Okay, show the dialogue
    show();
}

void updater::startSoftwareUpdate(void)
{
    // Disable potentially relevant buttons
    yesDoUpdate->setEnabled(false);
    noUpdate->setEnabled(false);

    totalBytesDownloaded = 0;
    downloadHash->reset();

    // Scale the progress bars
    downloadProgress->setRange(0,updateFileLength);
//    unpackProgress->setRange(0,updateFileLength);

    // Create an open file to copy downloaded data into
    QString filename = QDir::tempPath() + QDir::separator() + QString("CosnetaUpdate");

    if( ! QFile::exists(filename) )
    {
        QDir updateDir(filename);

        if( ! updateDir.mkdir(filename) )
        {
            balloonMessage(tr("Failed to create a temprory folder for downloading updates."));
            hide();

            return;
        }
    }

    filename = filename + QDir::separator() + QString(LOCAL_DOWNLOAD_BLOB_FILENAME);

    downloadBlobFile = new QFile(filename);

    // Set up the download
    if( downloadManager == NULL )
    {
        downloadManager = new QNetworkAccessManager;
    }

    QNetworkRequest request;
#if defined(Q_OS_WIN)
    request.setUrl(QUrl(WIN_DATA_URL));
#elif defined(Q_OS_LINUX)
    request.setUrl(QUrl(MAC_DATA_URL));
#elif defined(Q_OS_MAC)
    request.setUrl(QUrl(LINUX_DATA_URL));
#endif

    downloadRequestReply = downloadManager->get(request);

    connect(downloadManager,     SIGNAL(finished(QNetworkReply *)),this,SLOT(downloadFinished(QNetworkReply *)));
    connect(downloadRequestReply,SIGNAL(readyRead()),              this,SLOT(downloadDataAvailable()));

    // De-obfuscate as we receive the data, so set the key up
    XORPermutor = 0x73DDE298;

    downloadData.clear();

    // Set a flag
    downloadInProgress = true;
}

void updater::abortUpdate(void)
{
    hide();
}



#define TO_HEX(x,index) \
        (((x).constData())[index])&255,2,16,QLatin1Char('0')

void updater::extractFile(int fileNameLen, uchar *data, int dataLen)
{
    QString updateDirName = QDir::tempPath() + QDir::separator() + QString("CosnetaUpdate");
    QString extractedPath;
    QString filename;

    qDebug() << "Decompress. First 4 bytes:"
             << QString("%1%2%3%4").arg(TO_HEX(downloadData,0)).arg(TO_HEX(downloadData,1))
                                   .arg(TO_HEX(downloadData,2)).arg(TO_HEX(downloadData,3));

    QByteArray uncompressed     = qUncompress(data,dataLen);
    uchar     *uncompressedData = (uchar *)uncompressed.constData();

    qDebug() << "Embedded filename:" << QString::fromUtf8((char *)uncompressedData, fileNameLen);

    // We have enough data to chunk out a file
    extractedPath = QString::fromLatin1((char *)uncompressedData, fileNameLen);
    filename = updateDirName + QDir::separator() + extractedPath;

    // Look for subdirectories, and create as necessary
    if( QFile::exists(filename) )
    {
        qDebug() << "Output file" << filename << "exists. Delete it";
        QDir deletor;
        deletor.remove(filename);
    }
    else
    {
        qDebug() << "Extract to:" << filename;

        QStringList subdirs = extractedPath.split(QRegExp("[\\/]"),QString::SkipEmptyParts);

        if( subdirs.size() > 1 )
        {
            // We need to create the path - just deal with one level for now as it's all we need

            QString subDirPath = updateDirName + QDir::separator() + subdirs[0];

            if( ! QFile::exists(subDirPath) )
            {
                qDebug() << "Create subdir:" << subDirPath;

                QDir creator;

                creator.mkpath(subDirPath);
            }
        }
    }

    QFile comprFileOut(filename);

    if( comprFileOut.open(QIODevice::WriteOnly) )
    {
        comprFileOut.write(uncompressed.constData()+fileNameLen,uncompressed.size()-fileNameLen);
    }
    else
    {
        qDebug() << "Failed to store compressed file.";
    }

    comprFileOut.close();
}



void updater::downloadDataAvailable()
{
    // Add more data to the receive buffer
    QByteArray block = downloadRequestReply->readAll();

    downloadHash->addData(block);

    for(int i=0; i<block.size(); i++)
    {
        downloadData.append((int)(block[i]) ^ (int)nextXORByte());
    }

    totalBytesDownloaded += block.size();
    downloadProgress->setValue(totalBytesDownloaded);

    // Try and chunk the data out to file
    if( downloadData.size() < 4 ) return;

    int nextFileLen = (((unsigned char)downloadData[1]) << 16) +
                      (((unsigned char)downloadData[0]) <<  8) +
                       ((unsigned char)downloadData[2]);
    int strLen      = (unsigned char)(downloadData[3]);

    if( downloadData.size() >= nextFileLen )
    {
        extractFile(strLen, (uchar *)(downloadData.constData()+4), nextFileLen);

        downloadData.remove(0, 4+nextFileLen);
    }
}

void updater::downloadFinished(QNetworkReply *reply)
{
    bool success = true;

    QString infoMessage = tr("Download of update data complete. ");

    downloadInProgress = false;

    switch( reply->error() )
    {
    case QNetworkReply::NoError:

        qDebug() << "Download complete.";

        break;

    default:

        infoMessage += tr(" Failed to retreive software updates.");
        qDebug() << "Update check failed to download meta-data:" << reply->errorString();
        downloadData.clear();

        success = false;
    }

    QByteArray block = reply->readAll();

    downloadHash->addData(block);

    for(int i=0; i<block.size(); i++)
    {
        downloadData.append((int)(block[i]) ^ (int)nextXORByte());
    }

    totalBytesDownloaded += block.size();
    downloadProgress->setValue(totalBytesDownloaded);

    while( downloadData.size() > 3 )
    {
        int nextFileLen = (((unsigned char)downloadData[1]) << 16) +
                          (((unsigned char)downloadData[0]) <<  8) +
                           ((unsigned char)downloadData[2]);
        int strLen      = (unsigned char)(downloadData[3]);

        if( downloadData.size() >= nextFileLen )
        {
            extractFile(strLen, (uchar *)(downloadData.constData()+4), nextFileLen);

            downloadData.remove(0, 4+nextFileLen);

            // Loop again
        }
        else
        {
            infoMessage += QString(" Threw away %1 at end of file. Needed %2")
                                   .arg( downloadData.size() ).arg(nextFileLen);

            downloadData.clear();   // Force end of loop

            success = false;
        }
    }

    if( downloadData.size() > 0 )
    {
        infoMessage += QString(" Threw away %1 at end of file").arg(downloadData.size());

        if( downloadData.size()>3 )
        {
            int nextFileLen = (((unsigned char)downloadData[1]) << 16) +
                              (((unsigned char)downloadData[0]) <<  8) +
                               ((unsigned char)downloadData[2]);
            int strLen      = (unsigned char)(downloadData[3]);

            if( downloadData.size() > strLen+4 )
            {
                infoMessage += QString(" Threw away %1 at end of file '%2' which is %3 long")
                        .arg( downloadData.size() )
                        .arg( QString(QByteArray((char *)(downloadData.constData()+4),strLen)) )
                        .arg(nextFileLen);
            }
        }

        success = false;
    }

    // Verify the download

    if( updateShaString != QString(downloadHash->result().toHex()) )
    {
        qDebug() << "Download hash check failed:";
        qDebug() << "Received hash:  " << updateShaString;
        qDebug() << "Calculated hash:" << downloadHash->result().toHex();

        infoMessage += QString(" Download FAILED as data was corrupted.");
        success = false;
    }
    else
    {
        qDebug() << "Checksum verified download as good.";
    }

    hide();

    if( success )
    {
        // Write the metadata file (version + target folder)
        QString metaFilename = QDir::tempPath() + QDir::separator() + QString("CosnetaUpdate");
        metaFilename        += QDir::separator() + QString("version.dat");

        QFile ver(metaFilename);
        if( ver.open(QIODevice::WriteOnly|QIODevice::Truncate) )
        {
            ver.write((char *)&updateTimeStamp,sizeof(updateTimeStamp));

            QByteArray destDir = QCoreApplication::applicationDirPath().toUtf8();
            quint32    strLen = destDir.length();
            ver.write((char *)&strLen,sizeof(strLen));
            ver.write(destDir);

            ver.close();

            // Ask the user when to do the update
            QMessageBox::StandardButton press;
            press = QMessageBox::information(this,tr("Download complete."),
                                             tr("Ready for install. Do you wish to exit now and update"
                                                " the software now?\n"
                                                "Select 'no' to update when the program exits."),
                                             QMessageBox::Yes|QMessageBox::No);

            if( press == QMessageBox::Yes )
            {
                qDebug() << "Quit program selected...";

                quitProgram();
            }
        }
        else
        {
            QMessageBox::warning(this,tr("Update failed"), tr("Failed local file write."));
        }
    }
    else
    {
        qDebug() << infoMessage;
        QMessageBox::warning(this,tr("Download complete."),infoMessage);
    }
}


// The following code is the firmwareware update system. This shall be
// re-introduced later (like when we have hardware).
#if 0

// This is the method that will be used to reveal this dialogue.

void updater::startUpdate(void)
{
    // Could check for updates here, but not for now: keep last check status
    this->show();

    doUpdate->setEnabled(false);
    check->setEnabled(true);
#ifdef DEBUG_TAB
    resetSoftwareButton->setEnabled(true);
    updateBootloaderButton->setEnabled(true);
#endif
    closeButton->setEnabled(true);
}



updater::updater(QWidget *parent) : QWidget(parent)
{
    // Create the dialogue
    QVBoxLayout *updateLayout = new QVBoxLayout;

    check     = new QPushButton(tr("&Check for updates"),this);
    connect(check, SIGNAL(clicked()), this, SLOT(checkForNewSoftware()));
    updateLayout->addWidget(check);

    QVBoxLayout *updateBoxLayout = new QVBoxLayout;

    doUpdate            = new QPushButton(tr("&Apply updates"),this);
    QLabel *nameLabel   = new QLabel(tr("Update Tasks"));
    initUpdateLbl       = new QLabel(tr("* Initialise software download"));
    fetchUpdateLbl      = new QLabel(tr("* Fetch latest software"));
    applyUpdateLbl      = new QLabel(tr("* Download to device"));
    progress            = new QProgressBar;
    closeButton         = new QPushButton(tr("&Close"),this);

    progress->setRange(0,100);
    progress->setValue(0);

    connect(doUpdate,    SIGNAL(clicked()), this, SLOT(applyNewSoftwareUpdate()));
    connect(closeButton, SIGNAL(clicked()), this, SLOT(hide()));

    updateBoxLayout->addWidget(doUpdate);
    updateBoxLayout->addWidget(nameLabel);
    updateBoxLayout->addWidget(initUpdateLbl);
    updateBoxLayout->addWidget(fetchUpdateLbl);
    updateBoxLayout->addWidget(applyUpdateLbl);
    updateBoxLayout->addWidget(progress);

    updateBox = new QGroupBox;
    updateBox->setLayout( updateBoxLayout );
    updateBox->setEnabled( false );
    updateLayout->addWidget( updateBox );
#ifdef DEBUG_TAB
    resetSoftwareButton = new QPushButton(tr("&Reset embedded software"),this);
    connect(resetSoftwareButton, SIGNAL(clicked()), this, SLOT(resetEmbeddedSoftwareTable()));
    updateBootloaderButton = new QPushButton(tr("&Bootloader update"),this);
    connect(updateBootloaderButton, SIGNAL(clicked()), this, SLOT(downloadUpdatedBootloader()));

    QVBoxLayout *privateOptionsLayout = new QVBoxLayout;
    privateOptionsLayout->addWidget(resetSoftwareButton);
    privateOptionsLayout->addWidget(updateBootloaderButton);
    QGroupBox *privateOptionsBox = new QGroupBox("Not for release");
    privateOptionsBox->setLayout( privateOptionsLayout );
    updateLayout->addWidget( privateOptionsBox );
#endif
    updateLayout->addWidget(closeButton);

    setLayout(updateLayout);

    // And initialise any required variables
    updatesFound = false;

#ifdef NATIVE_ACCESS
#if defined(Q_OS_WIN)
    Handle = NULL;
#endif
#endif
}

updater::~updater(void)
{
    updaterCloseDevice();
}


// Catch the window exit click. We Just want to hide this window, not
// exit the application.

void updater::closeEvent(QCloseEvent *e)
{
    hide();

    e->ignore();
}




void updater::checkForNewSoftware(void)
{
    deviceType = UNKNOWN_DEVICE;

    // Retreive the current software running on flip
    if( updaterOpenDevice(COS_FLIP_DEV) )
    {
        deviceType = COS_FLIP_DEV;
    }
    else if( updaterOpenDevice(COS_FREERUNNER_DEV) )
    {
        deviceType = COS_FREERUNNER_DEV;
    }
    else
    {
        QMessageBox::warning(this, tr("No Flip or FreeRunner devices present."),
                    tr("A Flip device must be attached for update to occur."),
                    QMessageBox::Ok);

        updateBox->setEnabled( false );
        updateBox->setTitle(tr("No update possible."));
        return;
    }

    // Retreive latest versions of software on Flip device
    char      buffer[UPDATE_BLOCK_SZ];
    status_t *status = (status_t *)buffer;

    if( readUpdateBlock(buffer) )
    {
        status->latest_flip_firmware_version    = SWAP32(status->latest_flip_firmware_version);
        status->latest_pen_firmware_version     = SWAP32(status->latest_pen_firmware_version);
        status->latest_windows_software_version = SWAP32(status->latest_windows_software_version);
        status->latest_mac_software_version     = SWAP32(status->latest_mac_software_version);
        status->latest_linux_software_version   = SWAP32(status->latest_linux_software_version);

        qDebug() << "Current FLIP software versions (read status:" << (status->status == OKAY?"OKAY":"BAD") << ")";
        qDebug() << "Flip firmware version:   " << status->latest_flip_firmware_version;
        qDebug() << "Pen firmware version:    " << status->latest_pen_firmware_version;
        qDebug() << "Windows software version:" << status->latest_windows_software_version;
        qDebug() << "MAC software version:    " << status->latest_mac_software_version;
        qDebug() << "Linux software version:  " << status->latest_linux_software_version;
    }
    else
    {
        memset(buffer,0,UPDATE_BLOCK_SZ);
    }

    updaterCloseDevice();

    // Get the latest versions of the available software

    updateSize = 0;

    update_header_t serverVersions;
    if( ! retreiveLatestServerCodeVersions(&serverVersions) )
    {
        QMessageBox::warning(this, tr("Update server not found."),
                             tr("Failed to establish a connection to update server."),
                             QMessageBox::Ok);
    }
    else
    {
        updateSize = trimServerVersionsToUpdates(&serverVersions, status);
    }

    if( updateSize > 0 )
    {
        updateBox->setEnabled(true);
        if( deviceType == COS_FLIP_DEV ) updateBox->setTitle(tr("Flip &updates available."));
        else                             updateBox->setTitle(tr("Freerunner &updates available."));
    }
    else
    {
        updateBox->setEnabled(false);
        updateBox->setTitle(tr("No &updates found."));
    }
}


// TODO: This will hopefully change to use a web site or similar. When we have a
// secure server design, we can change this code to reflect it.
// In the meantime, it assumes tha the user has Dropbox in it's default location
// in the filesystem. It then does the retreival based on expected file locations.
// If no file at all is on the 'server', an available version (timestamp) of
// zero is returned for the element.

bool updater::retreiveLatestServerCodeVersions(update_header_t *serverVersions)
{
    // Get a directory for the update data.
    QString dataFilesDir = QDir::homePath()+QDir::separator()+"Dropbox"+QDir::separator()+
                           "Cosneta"+QDir::separator()+"Development"+QDir::separator()+
                           "Cosneta-John"+QDir::separator()+"Install_Software"+
                           QDir::separator()+"Firmware"+QDir::separator();
    QFile       readFile;
    QTextStream inStr;


    if( ! QFile::exists(dataFilesDir) )
    {
        dataFilesDir = QDir::homePath()+QDir::separator()+"Desktop"+QDir::separator()+
                       "CosnetaUpdate"+QDir::separator()+"Firmware"+QDir::separator();
        qDebug() << "Try desktop.";
    }

    readFile.setFileName(dataFilesDir+"flip_build.dat");

    serverVersions->flip_firmware_version = 0;
    serverVersions->flip_firmware_size    = 0;

    if( readFile.open(QIODevice::ReadOnly | QIODevice::Text) )
    {
        // We have opened the server version file for the flip build

        inStr.setDevice(&readFile);
        inStr >> serverVersions->flip_firmware_version;
        inStr >> serverVersions->flip_firmware_size;

        readFile.close();
    }

    readFile.setFileName(dataFilesDir+"freerunner_build.dat");

    serverVersions->pen_firmware_version = 0;
    serverVersions->pen_firmware_size    = 0;

    if( readFile.open(QIODevice::ReadOnly | QIODevice::Text) )
    {
        // We have opened the server version file for the pen build

        inStr.setDevice(&readFile);
        inStr >> serverVersions->pen_firmware_version;
        inStr >> serverVersions->pen_firmware_size;

        readFile.close();
    }

    dataFilesDir = QDir::homePath()+QDir::separator()+"Dropbox"+QDir::separator()+
                   "Cosneta"+QDir::separator()+"Development"+QDir::separator()+
                   "Cosneta-John"+QDir::separator()+"Install_Software"+
                   QDir::separator()+"HostSoftware"+QDir::separator();

    readFile.setFileName(dataFilesDir+"windows_build.dat");

    serverVersions->windows_software_version = 0;
    serverVersions->windows_software_size    = 0;

    if( readFile.open(QIODevice::ReadOnly | QIODevice::Text) )
    {
        // We have opened the server version file for the windows build

        inStr.setDevice(&readFile);
        inStr >> serverVersions->windows_software_version;
        inStr >> serverVersions->windows_software_size;

        readFile.close();
    }

    readFile.setFileName(dataFilesDir+"mac_build.dat");

    serverVersions->mac_software_version = 0;
    serverVersions->mac_software_size    = 0;

    if( readFile.open(QIODevice::ReadOnly | QIODevice::Text) )
    {
        // We have opened the server version file for the mac build

        inStr.setDevice(&readFile);
        inStr >> serverVersions->mac_software_version;
        inStr >> serverVersions->mac_software_size;

        readFile.close();
    }

    readFile.setFileName(dataFilesDir+"linux_build.dat");

    serverVersions->linux_software_version = 0;
    serverVersions->linux_software_size    = 0;

    if( readFile.open(QIODevice::ReadOnly | QIODevice::Text) )
    {
        // We have opened the server version file for the linux build

        inStr.setDevice(&readFile);
        inStr >> serverVersions->linux_software_version;
        inStr >> serverVersions->linux_software_size;

        readFile.close();
    }

    qDebug() << "Latest available software versions:";
    qDebug() << "Flip firmware version:   " << serverVersions->flip_firmware_version
             << "Length: "                  << serverVersions->flip_firmware_size;
    qDebug() << "Pen firmware version:    " << serverVersions->pen_firmware_version;
    qDebug() << "Windows software version:" << serverVersions->windows_software_version
             << "Length: "                  << serverVersions->windows_software_size;
    qDebug() << "MAC software version:    " << serverVersions->mac_software_version;
    qDebug() << "Linux software version:  " << serverVersions->linux_software_version;

    return true;
}


int updater::trimServerVersionsToUpdates(update_header_t *serverVersions, status_t *status)
{
    int updateSize = 0;

    if( deviceType == COS_FREERUNNER_DEV )
    {
        qDebug() << "For connected FreeRunner, mask out all updates except the FreeRunner.";

        serverVersions->flip_firmware_version    = 0;
        serverVersions->flip_firmware_size       = 0;

        // Pen - common code at the end.

        serverVersions->windows_software_version = 0;
        serverVersions->windows_software_size    = 0;

        serverVersions->mac_software_version     = 0;
        serverVersions->mac_software_size        = 0;

        serverVersions->linux_software_version   = 0;
        serverVersions->linux_software_size      = 0;
    }
    else
    {
        if( status->latest_flip_firmware_version   < serverVersions->flip_firmware_version &&
            serverVersions->flip_firmware_version != 0   )
        {
            updateSize += serverVersions->flip_firmware_size;
        }
        else
        {
            serverVersions->flip_firmware_version = 0;
            serverVersions->flip_firmware_size    = 0;
        }

        if( status->latest_windows_software_version    < serverVersions->windows_software_version &&
            serverVersions->windows_software_version != 0 )
        {
            updateSize += serverVersions->windows_software_size;
        }
        else
        {
            serverVersions->windows_software_version = 0;
            serverVersions->windows_software_size    = 0;
        }

        if( status->latest_mac_software_version    < serverVersions->mac_software_version &&
            serverVersions->mac_software_version != 0 )
        {
            updateSize += serverVersions->mac_software_size;
        }
        else
        {
            serverVersions->mac_software_version  = 0;
            serverVersions->mac_software_size     = 0;
        }

        if( status->latest_linux_software_version    < serverVersions->linux_software_version &&
            serverVersions->linux_software_version != 0 )
        {
            updateSize += serverVersions->linux_software_size;
        }
        else
        {
            serverVersions->linux_software_version = 0;
            serverVersions->linux_software_size    = 0;
        }
    }

    // We want pen updates to be available for all devices
    if( status->latest_pen_firmware_version   < serverVersions->pen_firmware_version &&
        serverVersions->pen_firmware_version != 0 )
    {
        updateSize += serverVersions->pen_firmware_size;
    }
    else
    {
        serverVersions->pen_firmware_version = 0;
        serverVersions->pen_firmware_size    = 0;
    }

    return updateSize;
}


static quint32 words[4] = {0,0,0,0};

// May change this to CRC32, but keep it simple and fast for now.
quint32 updater::computeCheckValue(QString filePath)
{
    quint32     checkVal = 0;
    double      sum = 0;
    int         bytesRead = 0;
    quint32     nextWord;
    QFile       readFile;

    readFile.setFileName(filePath);

    if( ! readFile.open(QIODevice::ReadOnly) )
    {
        qDebug() << "Failed checksum calculation for" << filePath;

        return 0;
    }

    while( sizeof(quint32) == readFile.read((char *)&nextWord, sizeof(quint32)) )
    {
        checkVal ^= nextWord;
        sum += nextWord;

        words[(bytesRead/4)&3] = nextWord;

        if( bytesRead != 0 )
        {
            if( (bytesRead & 15) == 0 )
            {
                qDebug() << QString("%1 %2 %3 %4 %5").arg(bytesRead,6,16)
                            .arg(words[0],8,16).arg(words[1],8,16)
                            .arg(words[2],8,16).arg(words[3],8,16);
            }

            if( bytesRead != 0 && (bytesRead & 511) == 0 )
            {
                qDebug() << " C(" << QString("0x%1").arg(checkVal,8,16) << ")";
            }
        }

        bytesRead += sizeof(quint32);
    }

    readFile.close();

    qDebug() << "Check value of" << filePath << "is" << checkVal << "sum=" << sum << " len=" << bytesRead;

    return checkVal;
}



// This update just does a file copy from Dropbox to the tmp directory. This is to
// replicate the final result of a network copy in the final system.
// Please excure the processEvents calls: I'll improve things when using networking.
bool updater::retrieveServerUpdateFiles(update_header_t *serverVersions)
{
    // Get a directory for the update data.
    QString dataFilesDir = QDir::homePath()+QDir::separator()+"Dropbox"+QDir::separator()+
                           "Cosneta"+QDir::separator()+"Development"+QDir::separator()+
                           "Cosneta-John"+QDir::separator()+"Install_Software"+
                           QDir::separator()+"Firmware"+QDir::separator();
    int bytesCopied = 0;

    if( ! QFile::exists(dataFilesDir) )
    {
        dataFilesDir = QDir::homePath()+QDir::separator()+"Desktop"+QDir::separator()+
                       "CosnetaUpdate"+QDir::separator()+"Firmware"+QDir::separator();
        qDebug() << "Try desktop.";
    }

    if( serverVersions->flip_firmware_version != 0 )
    {
        if( QFile::exists(QDir::tempPath()+QDir::separator()+"FLIP-at91sam9xe512-UPDATE.bin") )
        {
            // QFile::copy will not overwrite
            QFile::remove(QDir::tempPath()+QDir::separator()+"FLIP-at91sam9xe512-UPDATE.bin");
        }

        if( ! QFile::copy(dataFilesDir+QDir::separator()+"FLIP-at91sam9xe512-UPDATE.bin",
                          QDir::tempPath()+QDir::separator()+"FLIP-at91sam9xe512-UPDATE.bin") )
        {
            qDebug() << "Failed to retreive updated Flip firmware.";
            return false;
        }
        else
        {
            serverVersions->flip_firmware_csum = computeCheckValue(QDir::tempPath()+QDir::separator()+"FLIP-at91sam9xe512-UPDATE.bin");
        }

        bytesCopied += serverVersions->flip_firmware_size;

        qDebug() << "Server FLIP binary size:" << serverVersions->flip_firmware_size;
    }

    progress->setValue(bytesCopied);
    QCoreApplication::processEvents();

    if( serverVersions->pen_firmware_version != 0 )
    {
        if( QFile::exists(QDir::tempPath()+QDir::separator()+"FreeRunner-PEN-at91sam9xe512-UPDATE.bin") )
        {
            // QFile::copy will not overwrite
            QFile::remove(QDir::tempPath()+QDir::separator()+"FreeRunner-PEN-at91sam9xe512-UPDATE.bin");
        }

        if( ! QFile::copy(dataFilesDir+QDir::separator()+"FreeRunner-PEN-at91sam9xe512-UPDATE.bin",
                          QDir::tempPath()+QDir::separator()+"FreeRunner-PEN-at91sam9xe512-UPDATE.bin") )
        {
            qDebug() << "Failed to retreive updated FreeRunner firmware.";
            return false;
        }
        else
        {
            serverVersions->pen_firmware_csum = computeCheckValue(QDir::tempPath()+QDir::separator()+"FreeRunner-PEN-at91sam9xe512-UPDATE.bin");
            qDebug() << QString("Checksum of server data = 0x%1").arg(serverVersions->pen_firmware_csum,0,16);
        }

        bytesCopied += serverVersions->pen_firmware_size;

        qDebug() << "Server FreeRunner binary size:" << serverVersions->pen_firmware_size;
    }

    progress->setValue(bytesCopied);
    QCoreApplication::processEvents();

    dataFilesDir = QDir::homePath()+QDir::separator()+"Dropbox"+QDir::separator()+
                   "Cosneta"+QDir::separator()+"Development"+QDir::separator()+
                   "Cosneta-John"+QDir::separator()+"Install_Software"+
                   QDir::separator()+"HostSoftware"+QDir::separator();

    if( serverVersions->windows_software_version != 0 )
    {
        if( QFile::exists(QDir::tempPath()+QDir::separator()+"WINSOFTW.CAB") )
        {
            // QFile::copy will not overwrite
            QFile::remove(QDir::tempPath()+QDir::separator()+"WINSOFTW.CAB");
        }

        if( ! QFile::copy(dataFilesDir+QDir::separator()+"WINSOFTW.CAB",
                          QDir::tempPath()+QDir::separator()+"WINSOFTW.CAB") )
        {
            qDebug() << "Failed to retreive updated Windows host software.";
            return false;
        }
        else
        {
            serverVersions->windows_software_csum = computeCheckValue(QDir::tempPath()+QDir::separator()+"WINSOFTW.CAB");
        }

        bytesCopied += serverVersions->windows_software_size;

        qDebug() << "Server Windows software size:" << serverVersions->windows_software_size;
    }

    progress->setValue(bytesCopied);
    QCoreApplication::processEvents();

    if( serverVersions->mac_software_version != 0 )
    {
        if( QFile::exists(QDir::tempPath()+QDir::separator()+"MAC.TGZ") )
        {
            // QFile::copy will not overwrite
            QFile::remove(QDir::tempPath()+QDir::separator()+"MAC.TGZ");
        }

        if( ! QFile::copy(dataFilesDir+QDir::separator()+"MAC.TGZ",
                          QDir::tempPath()+QDir::separator()+"MAC.TGZ") )
        {
            qDebug() << "Failed to retreive updated MAC host software.";
            return false;
        }
        else
        {
            serverVersions->mac_software_csum = computeCheckValue(QDir::tempPath()+QDir::separator()+"MAC.TGZ");
        }

        bytesCopied += serverVersions->mac_software_size;

        qDebug() << "Server MAC software size:" << serverVersions->mac_software_size;
    }

    progress->setValue(bytesCopied);
    QCoreApplication::processEvents();

    if( serverVersions->linux_software_version != 0 )
    {
        if( QFile::exists(QDir::tempPath()+QDir::separator()+"LINUX.TGZ") )
        {
            // QFile::copy will not overwrite
            QFile::remove(QDir::tempPath()+QDir::separator()+"LINUX.TGZ");
        }

        if( ! QFile::copy(dataFilesDir+QDir::separator()+"LINUX.TGZ",
                          QDir::tempPath()+QDir::separator()+"LINUX.TGZ") )
        {
            qDebug() << "Failed to retreive updated Linux host software.";
            return false;
        }
        else
        {
            serverVersions->linux_software_csum = computeCheckValue(QDir::tempPath()+QDir::separator()+"LINUX.TGZ");
        }

        bytesCopied += serverVersions->linux_software_size;

        qDebug() << "Server Linux software size:" << serverVersions->linux_software_size;
    }

    progress->setValue(bytesCopied);
    QCoreApplication::processEvents();

    return true;
}


// Send the named file to the update block. The name is prepended with the
// name of the temprorary folder.
bool updater::sendSingleFileToUpdateFile(QString fileName, int bytesToSend)
{
    // Input file
    QFile       readFile;
    readFile.setFileName(QDir::tempPath()+QDir::separator()+fileName);

    // Destination
    if( ! updaterFSOpenDevice() )
    {
        qDebug() << "Failed to open update file on device.";

        balloonMessage(tr("Failed to open update file on device."));

        return false;
    }

    // Source
    if( ! readFile.open(QIODevice::ReadOnly) )
    {
        qDebug() << "Failed to open file:" << fileName;

        balloonMessage(tr("Failed to open file:")+fileName);

        return false;
    }

    if( bytesToSend != readFile.size() )
    {
        qDebug() << "bytesToSend[" << bytesToSend << "] != readFile.size() [" << readFile.size() << "]!!!";

//        bytesToSend = readFile.size();
    }

    // Data block to be sent
    char  buffer[UPDATE_BLOCK_SZ];
    int   numBytes;
    int   lastUpdateBytes = updateBytesDone;

    qDebug() << "Send" << bytesToSend << "bytes to Flip";

    while( bytesToSend > 0 )
    {
        numBytes = readFile.read(buffer, UPDATE_BLOCK_SZ);

        if( numBytes <= 0 || numBytes > UPDATE_BLOCK_SZ )
        {
            // Just leave this, padding out with the last block
            qDebug() << "Failed to read data block for" << fileName << "abort.";

            readFile.close();
            updaterFSCloseDevice();

            return false;
        }

        updateFile.write(buffer,numBytes);

        updateBytesDone += numBytes;
        bytesToSend     -= numBytes;

        /* Do this for a 200th of the complete update only */
        if( ((updateBytesDone - lastUpdateBytes)*1000)/updateSize > 5 )
        {
            qDebug() << "Bytes to send:" << bytesToSend << "Rate:" <<
                        (1000.0*updateBytesDone / timer.elapsed()) << "bytes/sec";
    
            progress->setValue(updateBytesDone);
            QCoreApplication::processEvents();

            msleep(10);

            lastUpdateBytes = updateBytesDone;
        }
    }

    readFile.close();

    updateFile.flush();
    updaterFSCloseDevice();

    // Attempt to persuade windows to actually send the data:
#ifdef Q_OS_WIN
    // Flush to device
    QString fname = getCosnetadevicePath().replace(2,1,QString("\\"));
    HANDLE hDev = CreateFile(fname.toStdWString().data(),
                             GENERIC_READ|GENERIC_WRITE,
                             FILE_SHARE_READ|FILE_SHARE_WRITE,
                             NULL, OPEN_EXISTING, FILE_FLAG_NO_BUFFERING, NULL);
    if( hDev == INVALID_HANDLE_VALUE )
    {
        qDebug() << QString("Failed to flush settings: failed to open %1 error=%2")
                    .arg(fname).arg(GetLastError());

        return true;
    }

    FlushFileBuffers(hDev);

    CloseHandle(hDev);
#endif


    msleep(1000);

    qDebug() << "Successfully sent file:" << fileName;

    return true;
}


// If this fails, as it's the last step, we don't bother to report the error.
// The user can try again, later. This method must only be called when the
// flip device has been opened.
void updater::applyUpdatesToDevice(update_header_t *serverVersions)
{
    // Tell the device what we are about to send
    char             buffer[UPDATE_BLOCK_SZ];
    update_header_t *header = (update_header_t *)buffer;
    update_result_t *results = (update_result_t *)buffer;

    memset(buffer,0,UPDATE_BLOCK_SZ);   // Clean out the block before sending

    memcpy(header, serverVersions, sizeof(update_header_t));

    header->start_sentinal        = HDR_START_SENTINAL;
    header->end_sentinal          = HDR_END_SENTINAL;

    qDebug() << "Write update header (expected versions) to Flip.";

    if( ! FSwriteUpdateBlock(buffer,sizeof(update_header_t)) )
    {
        balloonMessage(tr("Failed to tell Flip of the pending updates."));
        return;
    }

    qDebug() << "Update header done.";

    // For each file, keep sending 512 byte blocks until it's all been sent.
    // NB Use the header length rather than the actual file length.

    timer.start();

    // Send each file if there is an update available
    if( serverVersions->flip_firmware_version != 0 &&
       ! sendSingleFileToUpdateFile("FLIP-at91sam9xe512-UPDATE.bin",
                                    serverVersions->flip_firmware_size))
    {
        qDebug() << "Failed to send Flip firmware update.";
        balloonMessage(tr("Failed to send Flip firmware update."));
    }

    qDebug() << "Flip firmware done.";

    if( serverVersions->pen_firmware_version != 0 &&
       ! sendSingleFileToUpdateFile("FreeRunner-PEN-at91sam9xe512-UPDATE.bin",
                                    serverVersions->pen_firmware_size) )
    {
        qDebug() << "Failed to send FreeRunner firmware update.";
        balloonMessage(tr("Failed to send FreeRunner firmware update."));
    }

    qDebug() << "Pen firmware done.";

    if( serverVersions->windows_software_version != 0 &&
       ! sendSingleFileToUpdateFile("WINSOFTW.CAB",
                                    serverVersions->windows_software_size) )
    {
        qDebug() << "Failed to send Windows software update.";
        balloonMessage(tr("Failed to send Windows host software update."));
    }

    qDebug() << "Windows software done.";

    if( serverVersions->mac_software_version != 0 &&
       ! sendSingleFileToUpdateFile("MAC.TGZ",
                                    serverVersions->mac_software_size) )
    {
        qDebug() << "Failed to send MAC software update.";
        balloonMessage(tr("Failed to send MAC host software update."));
    }

    qDebug() << "MAC software done.";

    if( serverVersions->linux_software_version != 0 &&
       ! sendSingleFileToUpdateFile("LINUX.TGZ",
                                    serverVersions->linux_software_size) )
    {
        qDebug() << "Failed to send Linux software update.";
        balloonMessage(tr("Failed to send Linux host software update."));
    }

    // Check whether the device thinks it has succeeded.

    if( ! readUpdateBlock(buffer) )
    {
        qDebug() << "Failed to read result of update.";
    }
    else
    {
        qDebug() << "Results of update:";
        qDebug() << "Status:                         " << results->status;
        qDebug() << "Flip firmware update status:    " << results->flip_firmware_update_succeeded;
        qDebug() << "Pen firmware update status:     " << results->pen_firmware_update_succeeded;
        qDebug() << "Windows software update status: " << results->windows_software_update_succeeded;
        qDebug() << "MAC software update status:     " << results->mac_software_update_succeeded;
        qDebug() << "Linux software update status:   " << results->linux_software_update_succeeded;
    }
}



void updater::applyNewSoftwareUpdate(void)
{
    // //////////////////////////////////////////////////////////////////////////////////
    // What newer versions are available from the server & calculate progress bar scaling
    QString oldStyle = initUpdateLbl->styleSheet();
    initUpdateLbl->setStyleSheet("QLabel { font: bold }");

    // Retreive the current software running on flip
    if( ! updaterOpenDevice( deviceType ) )
    {
        QMessageBox::warning(this, tr("Cosneta device no longer present."),
                             tr("A Flip or FreeRunner device must be attached for update to occur."),
                             QMessageBox::Ok);

        updateBox->setTitle(tr("No &update possible."));
        updateBox->setEnabled( false );

        return;
    }

    if( ! updaterFSOpenDevice() )
    {
        QMessageBox::warning(this, tr("Failed FS open of Flip device."),
                             tr("A Flip device must be attached for update to occur."),
                             QMessageBox::Ok);

        updateBox->setTitle(tr("No &update possible."));
        updateBox->setEnabled( false );

        updaterCloseDevice();

        return;
    }

    updaterFSCloseDevice();

    check->setEnabled(false);
    doUpdate->setEnabled(false);
#ifdef DEBUG_TAB
    resetSoftwareButton->setEnabled(false);
    updateBootloaderButton->setEnabled(false);
#endif
    closeButton->setEnabled(false);

    // Retreive latest versions of software on Flip device
    char      buffer[UPDATE_BLOCK_SZ];
    status_t *status = (status_t *)buffer;

    if( readUpdateBlock(buffer) )
    {
        status->latest_flip_firmware_version    = SWAP32(status->latest_flip_firmware_version);
        status->latest_pen_firmware_version     = SWAP32(status->latest_pen_firmware_version);
        status->latest_windows_software_version = SWAP32(status->latest_windows_software_version);
        status->latest_mac_software_version     = SWAP32(status->latest_mac_software_version);
        status->latest_linux_software_version   = SWAP32(status->latest_linux_software_version);
    }
    else
    {
        updaterCloseDevice();

        balloonMessage(tr("Failed to access Flip device for update."));

        return;
    }

    // And the server versions available
    update_header_t serverVersions;

    if( ! retreiveLatestServerCodeVersions(&serverVersions) )
    {
        updaterCloseDevice();

        balloonMessage(tr("Failed to access server for update."));

        return;
    }

    // Trim to updates
    updateSize = trimServerVersionsToUpdates(&serverVersions, status);

    if( updateSize <= 0 )
    {
        initUpdateLbl->setStyleSheet(oldStyle);

        balloonMessage("Update no longer available.");

        updateBox->setTitle(tr("No &update possible."));
    }
    else
    {
        // Download the server code locally (just a copy, but will be a wget or some such)
        initUpdateLbl->setStyleSheet(oldStyle);
        oldStyle = fetchUpdateLbl->styleSheet();
        fetchUpdateLbl->setStyleSheet("QLabel { font: bold }");

        progress->setRange(0,updateSize);
        updateBytesDone = 0;

        // NB This next call also calculates a trivial check word
        if( ! retrieveServerUpdateFiles(&serverVersions) )
        {
            initUpdateLbl->setStyleSheet(oldStyle);

            balloonMessage("Failed to retreive update from server.");
        }
        else
        {
            // Download to Flip device
            fetchUpdateLbl->setStyleSheet(oldStyle);
            oldStyle = applyUpdateLbl->styleSheet();
            applyUpdateLbl->setStyleSheet("QLabel { font: bold }");

            updateBytesDone = 0;

            applyUpdatesToDevice(&serverVersions);
        }

        updateBox->setTitle(tr(""));
    }

    progress->setValue(0);

    // Done (re-enable the check button, should some wish to try again)
    applyUpdateLbl->setStyleSheet( oldStyle );

    check->setEnabled(true);
    doUpdate->setEnabled(false);
#ifdef DEBUG_TAB
    resetSoftwareButton->setEnabled(true);
    updateBootloaderButton->setEnabled(true);
#endif
    closeButton->setEnabled(true);

    updaterCloseDevice();
}



#ifdef DEBUG_TAB
void updater::resetEmbeddedSoftwareTable(void)
{
    char     buffer[UPDATE_BLOCK_SZ];
    qint32  *first = (int *)buffer;

    *first = RESET_COMMAND;
    strcpy(buffer+sizeof(qint32),"Reset the damned software table.");

    qDebug() << "Reset the stored software table on Flip.";

    if( ! FSwriteUpdateBlock(buffer,UPDATE_BLOCK_SZ) )
    {
        balloonMessage(tr("Failed to reset Flip software table."));
    }
    else
    {
        balloonMessage(tr("Reset Flip software table."));
    }
}

void updater::downloadUpdatedBootloader(void)
{
    char     buffer[UPDATE_BLOCK_SZ];
    qint32  *first = (int *)buffer;

    // Can we find the bootloader binary?

    QString dataFile = QDir::homePath()+QDir::separator()+"Desktop"+QDir::separator()+
                         "CosnetaUpdate"+QDir::separator()+"Firmware"+QDir::separator()+
                         "Bootloader-at91sam9xe512-flash.bin";

    if( ! QFile::exists(dataFile) )
    {
        QMessageBox::critical(this, tr("Bootloader update binary not found."),
                             tr("Failed to find binary for bootloader update. "
                                "It MUST be located in Desktop: "
                                "CosnetaUpdate//Firmware//Bootloader-at91sam9xe512-flash.bin"),
                             QMessageBox::Ok);
        return;
    }

    // Destination
    if( ! updaterFSOpenDevice() )
    {
        qDebug() << "Failed to open update file on device.";

        balloonMessage(tr("Failed to open update file on device."));

        return;
    }

    QFile readFile;
    readFile.setFileName(dataFile);

    if( ! readFile.open(QIODevice::ReadOnly) )
    {
        qDebug() << "Failed to open file:" << dataFile;

        balloonMessage(tr("Failed to open file:")+dataFile);

        return;
    }


    *first = DANGEROUS_UPDATES;
    strcpy(buffer+sizeof(qint32),"Update bootloader.");

    update_header_t *hdr  = (update_header_t *)(buffer+256);

    hdr->start_sentinal           = HDR_START_SENTINAL;
    hdr->end_sentinal             = HDR_END_SENTINAL;
    hdr->flip_firmware_version    = 0;
    hdr->windows_software_version = 0;
    hdr->mac_software_version     = 0;
    hdr->linux_software_version   = 0;
    hdr->pen_firmware_version     = DANGEROUS_UPDATES; // Just a flag as no version control
    hdr->pen_firmware_size        = readFile.size();
    hdr->pen_firmware_csum        = computeCheckValue(dataFile);

    qDebug() << "Update FreeRunner bootloader.";

    if( ! FSwriteUpdateBlock(buffer,UPDATE_BLOCK_SZ) )
    {
        balloonMessage(tr("Failed to update FreeRunner bootloader."));

        updaterFSCloseDevice();
        readFile.close();
    }
    else
    {
        qDebug("Started to update FreeRunner bootloader.");

        // Write the blocks to the update file.

        // Data block to be sent
        char  buffer[UPDATE_BLOCK_SZ];
        int   numBytes;
        int   bytesToSend = readFile.size();

        // Show progress (though hopefully this will be so fast as to missed)
        progress->setMinimum(0);
        progress->setMaximum(bytesToSend);
        progress->setValue(0);

        qDebug() << "Send" << bytesToSend << "bytes to Flip";

        while( bytesToSend > 0 )
        {
            numBytes = readFile.read(buffer, UPDATE_BLOCK_SZ);

            if( numBytes <= 0 || numBytes > UPDATE_BLOCK_SZ )
            {
                // Just leave this, padding out with the last block
                qDebug() << "Failed to read data block for" << dataFile << "abort.";

                readFile.close();
                updaterFSCloseDevice();

                return;
            }

            updateFile.write(buffer,numBytes);

            updateBytesDone += numBytes;
            bytesToSend     -= numBytes;

            /* Do the process events so it does get displayed by this thread. */
            progress->setValue(updateBytesDone);
            QCoreApplication::processEvents();
        }

        // And flush & clean up
        updaterFSCloseDevice();
        readFile.close();

        // Attempt to persuade windows to really really actually send the data:
#ifdef Q_OS_WIN
        // Flush to device
        QString fname = getCosnetadevicePath().replace(2,1,QString("\\"));
        HANDLE hDev = CreateFile(fname.toStdWString().data(),
                                 GENERIC_READ|GENERIC_WRITE,
                                 FILE_SHARE_READ|FILE_SHARE_WRITE,
                                 NULL, OPEN_EXISTING, FILE_FLAG_NO_BUFFERING, NULL);
        if( hDev == INVALID_HANDLE_VALUE )
        {
            qDebug() << QString("Failed to flush settings: failed to open %1 error=%2")
                        .arg(fname).arg(GetLastError());

            return;
        }

        FlushFileBuffers(hDev);

        CloseHandle(hDev);
#endif
        // Announce completion
        balloonMessage(tr("Updated FreeRunner bootloader. Reboot it to test."));
    }
}

#endif




#ifdef NATIVE_ACCESS

/* SCSI data */
#define READ10           0x28
#define WRITE10          0x2A
#define TXFER_COUNT      1			/* Low latency */
#define SCSI_BLOCKSIZE   512

#ifdef Q_OS_WIN

#define FREERUNNER_PID L"PID_612B"
#define FLIP_PID       L"PID_612A"

bool updater::updaterOpenDevice(cosneta_device_t devType)
{
    HDEVINFO		                 hDevInfo;
    SP_DEVINFO_DATA	                 devInfoData;
    WCHAR                            deviceIDStr[MAX_DEVICE_ID_LEN];
    SP_DEVICE_INTERFACE_DATA         devIfaceData;
    SP_DEVICE_INTERFACE_DETAIL_DATA *devIfaceDetailData_p;

    devIfaceDetailData_p=(SP_DEVICE_INTERFACE_DETAIL_DATA *)calloc(4+1024,sizeof(WCHAR));
    if (devIfaceDetailData_p == NULL)
    {
        updaterFSCloseDevice();
        return false;
    }

    hDevInfo = SetupDiGetClassDevs(&GUID_DEVINTERFACE_DISK, NULL, NULL, DIGCF_PRESENT|DIGCF_DEVICEINTERFACE);

    if (hDevInfo == INVALID_HANDLE_VALUE)
    {
        updaterFSCloseDevice();
        return false;
    }

    int  idx=0;
    devInfoData.cbSize = sizeof(devInfoData);
    while( SetupDiEnumDeviceInfo(hDevInfo, idx, &devInfoData) )
    {
        /* Is it the cosneta device? (NB This is Win7 and up only) */
        if( CR_SUCCESS == CM_Get_Device_ID(devInfoData.DevInst, deviceIDStr, MAX_DEVICE_ID_LEN, 0) )
        {
            if( wcsstr(deviceIDStr, L"USBSTOR\\DISK&VEN_COSNETA") )
            {
                cosneta_device_t foundDevType = UNKNOWN_DEVICE;
                DEVINST          parentDevInst;

                if( CR_SUCCESS == CM_Get_Parent(&parentDevInst, devInfoData.DevInst, 0) )
                {
                    WCHAR    parentIDStr[MAX_DEVICE_ID_LEN];
                    if( CR_SUCCESS == CM_Get_Device_ID(parentDevInst, parentIDStr, MAX_DEVICE_ID_LEN, 0) )
                    {
                        qDebug() << "Parent device string:" << QString::fromWCharArray(parentIDStr);

                        // Device 612A is a Flip, and 612B is a FreeRunner
                        if( wcsstr(parentIDStr, FREERUNNER_PID) )
                        {
                            qDebug() << "Device" <<  QString::fromWCharArray(parentIDStr) << "is a Freerunner.";

                            foundDevType = COS_FREERUNNER_DEV;
                        }
                        else if( wcsstr(parentIDStr, FLIP_PID) )
                        {
                            qDebug() << "Flip found.";

                            foundDevType = COS_FLIP_DEV;
                        }
                    }
                }


                /* Device found with correct manufacturer ID. Is it the correct product? */
                if( foundDevType == devType )
                {
                    devIfaceData.cbSize          = sizeof(devIfaceData);
                    devIfaceDetailData_p->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);
                    if ( SetupDiEnumDeviceInterfaces(hDevInfo, NULL, &GUID_DEVINTERFACE_DISK, idx, &devIfaceData) )
                    {
                        if ( SetupDiGetDeviceInterfaceDetail(hDevInfo, &devIfaceData,
                                                             devIfaceDetailData_p, (4+1024)*sizeof(WCHAR),
                                                             NULL, NULL )                          )
                        {
                            /* Open the device and leave the handle in the global "Handle" */
                            HANDLE hDev = CreateFile(devIfaceDetailData_p->DevicePath,
                                                     GENERIC_READ|GENERIC_WRITE, FILE_SHARE_READ|FILE_SHARE_WRITE,
                                                     NULL, OPEN_EXISTING, FILE_FLAG_NO_BUFFERING|FILE_FLAG_WRITE_THROUGH , NULL);

                            if( hDev != INVALID_HANDLE_VALUE )
                            {
                                // EXIT here on success!!!
                                Handle = hDev;

                                SetupDiDestroyDeviceInfoList(hDevInfo);
                                free(devIfaceDetailData_p);

                                return true;        // ********** EXIT HERE!!!! **********
                            }
#ifdef DEBUG_TAB
                            else
                            {
                                balloonMessage(QString( "Failed to CreateFile for Cosneta device.\nErr(5=> access denied): %1").arg(GetLastError()));
                            }
#endif
                        }
#ifdef DEBUG_TAB
                        else
                        {
                            balloonMessage(QString("Failed to extract Cosneta device file path.\nFailed SetupDiGetDeviceInterfaceDetail()\nErr= ").arg(GetLastError()));
                        }
#endif
                    }
#ifdef DEBUG_TAB
                    else
                    {
                        balloonMessage(QString("Failed to extract device file path.\nFailed SetupDiEnumDeviceInterfaces()\nErr=%1").arg(GetLastError()));
                    }
#endif
                    //	Failed, but keep iteterating in case there is a similar device that does work
                }
            }
#if 1
            else
            {
                qDebug() << "Not a Cosneta product. Path:" << QString::fromWCharArray(deviceIDStr);
            }
#endif
        }

        /* And next in list of devices */
        devInfoData.cbSize = sizeof(devInfoData);
        idx ++;
    }

    SetupDiDestroyDeviceInfoList(hDevInfo);

    free(devIfaceDetailData_p);

    updaterFSCloseDevice();

    return false;
}


void updater::updaterCloseDevice(void)
{
    if( Handle != NULL )
    {
        CloseHandle( Handle );
        Handle = NULL;
    }
}

bool updater::readUpdateBlock(char *buffer)
{
    static unsigned char      dataBuffer[sizeof(SCSI_PASS_THROUGH) + UPDATE_BLOCK_SZ] = { 0 };
    static SCSI_PASS_THROUGH *spt = (SCSI_PASS_THROUGH *)dataBuffer;

    spt->Length             = sizeof(SCSI_PASS_THROUGH);
    spt->PathId             = 0;
    spt->TargetId           = 1;
    spt->Lun                = 0;
    spt->CdbLength          = 10;
    spt->SenseInfoLength    = 0;
    spt->DataIn             = SCSI_IOCTL_DATA_IN;			// Read data FROM the device
    spt->DataTransferLength = TXFER_COUNT*SCSI_BLOCKSIZE;
    spt->TimeOutValue       = 2;							// Seconds
    spt->DataBufferOffset   = spt->Length;
    spt->SenseInfoOffset    = 0;

    spt->Cdb[0] =  READ10;  // 0x28
    spt->Cdb[1] =  0;
    spt->Cdb[2] = (UPDATE_BLK >> 24) & 255;
    spt->Cdb[3] = (UPDATE_BLK >> 16) & 255;
    spt->Cdb[4] = (UPDATE_BLK >> 8)  & 255;
    spt->Cdb[5] =  UPDATE_BLK        & 255;
    spt->Cdb[6] =  0;
    spt->Cdb[7] = (TXFER_COUNT >> 8) & 255;
    spt->Cdb[8] =  TXFER_COUNT       & 255;
    spt->Cdb[9] =  0;
//memset(dataBuffer+sizeof(SCSI_PASS_THROUGH),0xff,SCSI_BLOCKSIZE);
    DWORD dwBytesReturned=0;

    qDebug() << "readUpdateBlock: call DeviceIoControl()";

    if ( ! DeviceIoControl(Handle, IOCTL_SCSI_PASS_THROUGH,
                           spt, spt->Length,							// inbuffer,  inBufSize
                           spt, spt->Length+spt->DataTransferLength,	// outbuffer, outBufSize
                           &dwBytesReturned, NULL) )
    {
        qDebug() << "Failed to read Freerunner data. Error=" << GetLastError();

        return false;
    }

    if ( dwBytesReturned < spt->Length )
    {
        qDebug() <<  "Error in reading Flip data: dwBytesReturned(" << dwBytesReturned <<
                     ") < spt->Length (" << spt->Length;
        return false;
    }

    // Copy out pointer to the returned data.
    memcpy(buffer,&(dataBuffer[sizeof(SCSI_PASS_THROUGH)]),SCSI_BLOCKSIZE);

    return true;
}

#if 0
bool updater::writeUpdateBlock(char *sendData, int length)
{
#if 1
//    static unsigned char      dataBuffer[sizeof(SCSI_PASS_THROUGH_DIRECT) + UPDATE_BLOCK_SZ] = { 0 };
//    SCSI_PASS_THROUGH_DIRECT *command = (SCSI_PASS_THROUGH_DIRECT *)dataBuffer;
    SCSI_PASS_THROUGH_DIRECT command;

    /* Fill in the databuffer... */
#if 0
    memset(command,0,sizeof(SCSI_PASS_THROUGH_DIRECT));

    command->Length             = sizeof(SCSI_PASS_THROUGH_DIRECT);
    command->PathId             = 0;
    command->TargetId           = 1;
    command->Lun                = 0;
    command->CdbLength          = 10;                           // Strangely, CDBs for READ10 & WRITE10 are 10 long.
    command->SenseInfoLength    = 0;
    command->DataIn             = SCSI_IOCTL_DATA_OUT;			// Write data TO the device
    command->DataTransferLength = TXFER_COUNT*SCSI_BLOCKSIZE;
    command->TimeOutValue       = 2;							// Seconds
    //    command->SenseInfoOffset    = (ULONG)(dataBuffer+sizeof(SCSI_PASS_THROUGH_DIRECT)); // dataBuffer+sizeof(SCSI_PASS_THROUGH);
    //    command->SenseInfoOffset    = sizeof(SCSI_PASS_THROUGH_DIRECT);
    command->DataBuffer         = dataBuffer+sizeof(SCSI_PASS_THROUGH_DIRECT);

    command->Cdb[0] =  WRITE10;  // 0x2A
    command->Cdb[1] =  0;
    command->Cdb[2] = (UPDATE_BLK >> 24) & 255;
    command->Cdb[3] = (UPDATE_BLK >> 16) & 255;
    command->Cdb[4] = (UPDATE_BLK >> 8)  & 255;
    command->Cdb[5] =  UPDATE_BLK        & 255;
    command->Cdb[6] =  0;
    command->Cdb[7] = (TXFER_COUNT >> 8) & 255;
    command->Cdb[8] =  TXFER_COUNT       & 255;
    command->Cdb[9] =  0;

    if( length <= 0 || length > UPDATE_BLOCK_SZ ) length = UPDATE_BLOCK_SZ;

    memcpy(dataBuffer+sizeof(SCSI_PASS_THROUGH_DIRECT), sendData, length);
#else
    memset(&command,0,sizeof(SCSI_PASS_THROUGH_DIRECT));

    command.Length             = sizeof(SCSI_PASS_THROUGH_DIRECT);
    command.PathId             = 0;
    command.TargetId           = 1;
    command.Lun                = 0;
    command.CdbLength          = 10;                           // Strangely, CDBs for READ10 & WRITE10 are 10 long.
    command.SenseInfoLength    = 0;
    command.DataIn             = SCSI_IOCTL_DATA_OUT;			// Write data TO the device
    command.DataTransferLength = TXFER_COUNT*SCSI_BLOCKSIZE;
    command.TimeOutValue       = 2;							// Seconds
    command.DataBuffer         = sendData;

    command.Cdb[0] =  WRITE10;  // 0x2A
    command.Cdb[1] =  0;
    command.Cdb[2] = (UPDATE_BLK >> 24) & 255;
    command.Cdb[3] = (UPDATE_BLK >> 16) & 255;
    command.Cdb[4] = (UPDATE_BLK >> 8)  & 255;
    command.Cdb[5] =  UPDATE_BLK        & 255;
    command.Cdb[6] =  0;
    command.Cdb[7] = (TXFER_COUNT >> 8) & 255;
    command.Cdb[8] =  TXFER_COUNT       & 255;
    command.Cdb[9] =  0;
#endif

    DWORD dwBytesReturned, status;

    qDebug() << "writeUpdateBlock: call DeviceIoControl()";

    status = DeviceIoControl(Handle, IOCTL_SCSI_PASS_THROUGH_DIRECT,
                             &command, sizeof(command),	// inbuffer,  inBufSize
                             &command, sizeof(command),	// outbuffer, outBufSize
                             &dwBytesReturned, NULL);

    if( status == 0 && command.ScsiStatus == 0 )
    {
        qDebug() << "SCSI write SUCCEEDED.";
        return true;
    }

    qDebug() << "SCSI write block failed in some way.";

    return false;
#else
    unsigned char     *dataBuffer;
    SCSI_PASS_THROUGH *spt;
    int                dataBufferSize;

    dataBufferSize = sizeof(SCSI_PASS_THROUGH) + UPDATE_BLOCK_SZ; // spt + rounded up number of blocks

    dataBuffer = (unsigned char *)calloc(dataBufferSize,sizeof(char));

    if( ! dataBuffer )
    {
        qDebug() << "Failed to allocate SCSI transmit buffer";
        return false;
    }

    /* Fill in the databuffer... */
    memset(dataBuffer,0,dataBufferSize);

    /* Fill in SCSI control stuff */
    spt = (SCSI_PASS_THROUGH *)dataBuffer;

    spt->Length             = sizeof(SCSI_PASS_THROUGH);
    spt->PathId             = 0;
    spt->TargetId           = 1;
    spt->Lun                = 0;
    spt->CdbLength          = 10;
    spt->SenseInfoLength    = 0;
    spt->DataIn             = SCSI_IOCTL_DATA_OUT;			// Write data TO the device
    spt->DataTransferLength = TXFER_COUNT*SCSI_BLOCKSIZE;
    spt->TimeOutValue       = 2;							// Seconds
    spt->DataBufferOffset   = spt->Length;
    spt->SenseInfoOffset    = 0;

    spt->Cdb[0] =  WRITE10;  // 0x2A
    spt->Cdb[1] =  0;
    spt->Cdb[2] = (UPDATE_BLK >> 24) & 255;
    spt->Cdb[3] = (UPDATE_BLK >> 16) & 255;
    spt->Cdb[4] = (UPDATE_BLK >> 8)  & 255;
    spt->Cdb[5] =  UPDATE_BLK        & 255;
    spt->Cdb[6] =  0;
    spt->Cdb[7] = (TXFER_COUNT >> 8) & 255;
    spt->Cdb[8] =  TXFER_COUNT       & 255;
    spt->Cdb[9] =  0;

    /* And copy in the data to send */
    dataBuffer[1+sizeof(SCSI_PASS_THROUGH)] = cmd;

    memcpy(dataBuffer+1+sizeof(SCSI_PASS_THROUGH), buf, buffSize);

    /* Now use the Windaes API to send it. */

    DWORD dwBytesReturned=0;

    if ( ! DeviceIoControl(Handle, IOCTL_SCSI_PASS_THROUGH,
                           spt, spt->Length,							// inbuffer,  inBufSize
                           spt, spt->Length+spt->DataTransferLength,	// outbuffer, outBufSize
                           &dwBytesReturned, NULL) )
    {
        qDebug() << "Failed to write data to Freerunner. Error=" << GetLastError();

        free(dataBuffer);

        return false;
    }

    /* No interest in any return. Clean up and return. */

    free(dataBuffer);

    return true;
#endif
}
#endif

#elif defined(Q_OS_LINUX)

#include <unistd.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <scsi/scsi.h>
#include <scsi/sg.h>

// Sigh...
void c_close(int fd)
{
    close(fd);
}

bool updater::updaterOpenDevice(cosneta_device_t devType)
{
    char               *dev_path = "/dev/freerunner";

    qDebug() << "TODO: write code to identify device type and open the raw device.";

    if( cosneta_fd >= 0 )
    {
        c_close( cosneta_fd );
    }

    cosneta_fd = open(dev_path, O_RDWR);

    // Open the device for use
    if( cosneta_fd < 0 )
    {
        qDebug() << "Failed to open the Cosneta device!";

        return false;
    }

    qDebug() << "updater: Freerunner device opened.";

    return true;
}




void updater::updaterCloseDevice(void)
{
    if( cosneta_fd >= 0 )
    {
        c_close( cosneta_fd );
        cosneta_fd = -1;
    }
}

bool updater::readUpdateBlock(char *buffer)
{
    sg_io_hdr_t            io;
    unsigned char          sense_b[SG_MAX_SENSE];		// Status info from ioctl (old driver max size, hopefully okay)
    unsigned char          scsiRead10[10] = {
                                             READ_10,               // Command
                                             0,                     // Flags: WRPROTECT [3], DPO [1], FUA [1], Reserved [1], FUA_NV [1], Obsolete [1]
                                             (UPDATE_BLK >> 24) & 255,// LBA (address) 5555dec = 15b3h
                                             (UPDATE_BLK >> 16) & 255,
                                             (UPDATE_BLK >> 8)  & 255,
                                              UPDATE_BLK        & 255,
                                             0,                     // Group number
                                             (TXFER_COUNT >> 8) & 255,
                                              TXFER_COUNT       & 255,	       // Length (read 1 * LS_SIZE(512) bytes = 512)
                                             0                      // Control
                                            };
    if( cosneta_fd < 0 )
    {
        qDebug() << "updater::readUpdateBlock quit as no open file descriptor!";

        return false;
    }

    bzero(&io, sizeof(io));


    // Fill in the io header for this scsi read command
    io.interface_id    = 'S';			            // Always set to 'S' for sg driver
    io.cmd_len         = sizeof(scsiRead10);	    // Size of SCSI command
    io.cmdp            = scsiRead10;		        // SCSI command buffer
    io.dxfer_direction = SG_DXFER_FROM_DEV;		    // Data transfer direction(no data) - -ve => new interface FROM => read
    io.dxfer_len       = SCSI_BLOCKSIZE;	        // byte count of data transfer (blk size is 512)
    io.dxferp          = buffer;		            // Data transfer buffer
    io.sbp             = sense_b;			        // Sense buffer
    io.mx_sb_len       = sizeof(sense_b);		    // Max sense buffer size (for error)
    io.flags           = SG_FLAG_DIRECT_IO;		    // Request direct IO (should still work if not honoured)
    io.timeout         = 5000;			            // Timeout (5s)

    // Read the next packet (NB doesn't read past the end of a line in a single action)
    int ret = ioctl(cosneta_fd, SG_IO, &io);
    if( ret < 0 )
    {
        qDebug() << "updater::readUpdateBlock exit as ioctl failed on read! Ret = "+QString::number(ret)+"\nreadThread: ("+QString::number(errno)+" 25=ENOTTY) ";

        return false;
    }
    if( io.masked_status != 0 ) /* ie not GOOD */
    {
        QString genericDebugString = "updater::readUpdateBlock masked_status = ";
        switch( io.masked_status )
        {
        case 0x01: genericDebugString+="CHECK_CONDITION";		break;
        case 0x02: genericDebugString+="CONDITION_GOOD";		break;
        case 0x04: genericDebugString+="BUSY";                  break;
        case 0x08: genericDebugString+="INTERMEDIATE_GOOD";     break;
        case 0x0a: genericDebugString+="INTERMEDIATE_C_GOOD"; 	break;
        case 0x0c: genericDebugString+="RESERVATION_CONFLICT";	break;
        case 0x11: genericDebugString+="COMMAND_TERMINATED";  	break;

        default:   genericDebugString+="Unknown ("+QString::number(io.masked_status)+")";
        }

        qDebug() << genericDebugString;

        return false;
    }

    return true;
}


#endif

#else

// File system access (not native)

bool updater::readUpdateBlock(char *buffer)
{
}

#endif

bool updater::FSwriteUpdateBlock(char *sendData, int length)
{
    QFile   updateBlock;
    QString updateFileName = getCosnetadevicePath() + "COSNETA/DEV/UPDATERD.DAT";

    updateBlock.setFileName(updateFileName);

    if( ! updateBlock.open(QIODevice::Unbuffered|QIODevice::ReadWrite) )
    {
        balloonMessage("Failed to open the updaterd file. Update failed.");
        qDebug() << "Failed to open the update file. Update failed.";

        return false;
    }
#if 0
    uchar *mappedData = updateBlock.map(0,length);
    if( mappedData == 0 )
    {
        balloonMessage("Failed to memory map the update file. Update failed.");
        msleep(1000);

        updateBlock.close();

        return false;
    }

    memcpy(mappedData,sendData,length);

    updateBlock.flush();
#ifdef Q_OS_WIN
    // Windows handle for win32 API: (HANDLE)_get_osfhandle(updateBlock.handle())
    FlushViewOfFile(mappedData,length);
#endif
    updateBlock.unmap(mappedData);
#else
    int sent = updateBlock.write(sendData,length);

    qDebug() << "Sent" << sent << "bytes to update block (done =" << updateBytesDone << ")";
#endif
    updateBlock.close();
#ifdef Q_OS_WIN
    // Flush to device
    QString fname = getCosnetadevicePath().replace(2,1,QString("\\"));
    HANDLE hDev = CreateFile(fname.toStdWString().data(),
                             GENERIC_READ|GENERIC_WRITE,
                             FILE_SHARE_READ|FILE_SHARE_WRITE,
                             NULL, OPEN_EXISTING, FILE_FLAG_NO_BUFFERING, NULL);
    if( hDev == INVALID_HANDLE_VALUE )
    {
        qDebug() << QString("Failed to flush settings: failed to open %1 error=%2")
                    .arg(fname).arg(GetLastError());

        return true;
    }

    FlushFileBuffers(hDev);

    CloseHandle(hDev);
#endif
    return true;
}

void updater::updaterFSCloseDevice(void)
{
    updateFile.close();
#ifdef Q_OS_WIN
    // Flush to device
    QString fname = getCosnetadevicePath().replace(2,1,QString("\\"));
    HANDLE hDev = CreateFile(fname.toStdWString().data(),
                             GENERIC_READ|GENERIC_WRITE,
                             FILE_SHARE_READ|FILE_SHARE_WRITE,
                             NULL, OPEN_EXISTING, FILE_FLAG_NO_BUFFERING, NULL);
    if( hDev == INVALID_HANDLE_VALUE )
    {
        qDebug() << QString("Failed to flush settings: failed to open %1 error=%2")
                    .arg(fname).arg(GetLastError());

        return;
    }

    FlushFileBuffers(hDev);

    CloseHandle(hDev);
#endif
}


bool updater::updaterFSOpenDevice(void)
{
    QString updateFileName = getCosnetadevicePath() + "COSNETA/DEV/UPDATEWR.DAT";

    if( updateFile.isOpen() ) updaterFSCloseDevice();

    if( QFile::exists(updateFileName) )
    {
        updateFile.setFileName(updateFileName);

        if( updateFile.open(QIODevice::Unbuffered|QIODevice::WriteOnly|
                            QIODevice::Truncate) )
        {
            return true;
        }
        else
        {
            qDebug() << "Failed to open existing file:" << updateFileName;
        }
    }
    else
    {
        qDebug() << "Could not find:" << updateFileName;
    }

    return false;
}

#endif
// End of hardware update code
