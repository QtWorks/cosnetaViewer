
#include <QtGui>
#include <QPoint>
#include <QApplication>
#include <QProcess>
#include <QTime>
#include <iostream>
#include <QMessageBox>

#include "GUI.h"
#include "pen_location.h"
#include "appComms.h"
#include "main.h"
#include "FreerunnerReader.h"
#include "invensenseReader.h"
#include "dongleReader.h"
#include "networkReceiver.h"
#include "licenseManager.h"
#include "main.h"
#include "savedSettings.h"
#include "radarResponder.h"

// Accessible to the GUI so that we can apply settings
int  mouseSensitivity          = 50;     // 0-100 scale TODO: remove this
#ifdef DEBUG_TAB
bool generateSampleData           = false;
bool useMouseData                 = false;
#endif
bool    freerunnerPointerDriveOn  = true;
bool    freerunnerGesturesDriveOn = true;

// Stored session settings. Add any more here.
bool               autoUpdateEnabled;
QDate              dateOfLastUpdateCheck;

#ifdef _WIN32

#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
// Windows Header Files:
#include <windows.h>

#endif


QString locationDebugString = "Unset";
extern QString gestureDebugString;

#ifdef Q_OS_WIN

#define WIN_AUTOSTART_DIR "/AppData/Roaming/Microsoft/Windows/Start Menu/Programs/Startup/"
bool    willAutoStart(void)
{
    QString startDir = QDir::homePath() + WIN_AUTOSTART_DIR;

    if( QFile::exists(startDir) )
    {
        // Is there a link to this executable?

        if( QFile::exists(startDir+"/sysTray.exe.lnk") )
        {
            QFile linkedFile(startDir+"/sysTray.exe.lnk");

            if( linkedFile.symLinkTarget() == QApplication::applicationFilePath() )
            {
                return true;
            }
        }
    }
    else
    {
        qDebug() << "Start directory doesn't exist";

        return false;
    }

    return false;
}

void    setAutoStart(bool start)
{
    QString startDir = QDir::homePath() + WIN_AUTOSTART_DIR;

    if( ! QFile::exists(startDir) )
    {
        // Is there a link to this executable?

        QDir d;
        if( ! d.mkpath(startDir) ) qDebug() << "Failed to create start directory.";

        qDebug() << "Created startup directory";
    }

    // Now let's do the deed
    if( start )
    {
        QFile thisExe(QApplication::applicationFilePath());
        if( ! thisExe.link(startDir+"/sysTray.exe.lnk") )
            qDebug() << "Failed to link to this executable.";
    }
    else
    {
        QFile linkedFile(startDir+"/sysTray.exe.lnk");
        if( linkedFile.exists() )
        {
            if( ! linkedFile.remove() )
                qDebug() << "Failed to remove autostart link";
        }
    }
}


void sendKeypressToOS(int keyWithModifier, bool withShift, bool withControl, bool withAlt)
{
    INPUT   in;
    WORD    key;

    switch( keyWithModifier )
    {
    case Qt::Key_Enter:     key = VK_RETURN;    qDebug() << "Key_Enter";  break;
    case Qt::Key_Return:    key = VK_RETURN;    qDebug() << "Key_Return"; break;
    case Qt::Key_Escape:    key = VK_ESCAPE;    break;
    case Qt::Key_Up:        key = VK_UP;        break;
    case Qt::Key_Down:      key = VK_DOWN;      break;
    case Qt::Key_Left:      key = VK_LEFT;      break;
    case Qt::Key_Right:     key = VK_RIGHT;     break;
    case Qt::Key_Delete:    key = VK_DELETE;    break;
    case Qt::Key_Backspace: key = VK_BACK;      break;
    case Qt::Key_Tab:       key = VK_TAB;       break;
    case Qt::Key_PageUp:    key = VK_PRIOR;     break;
    case Qt::Key_PageDown:  key = VK_NEXT;      break;
    case Qt::Key_Home:      key = VK_HOME;      break;
    case Qt::Key_End:       key = VK_END;       break;
    case Qt::Key_Insert:    key = VK_INSERT;    break;
    case Qt::Key_F1:        key = VK_F1;        break;
    case Qt::Key_F2:        key = VK_F2;        break;
    case Qt::Key_F3:        key = VK_F3;        break;
    case Qt::Key_F4:        key = VK_F4;        break;
    case Qt::Key_F5:        key = VK_F5;        break;
    case Qt::Key_F6:        key = VK_F6;        break;
    case Qt::Key_F7:        key = VK_F7;        break;
    case Qt::Key_F8:        key = VK_F8;        break;
    case Qt::Key_F9:        key = VK_F9;        break;
    case Qt::Key_F10:       key = VK_F10;       break;
    case Qt::Key_F11:       key = VK_F11;       break;
    case Qt::Key_F12:       key = VK_F12;       break;
    case Qt::Key_Minus:     key = VK_SUBTRACT;  break;
    case Qt::Key_Slash:     key = VK_DIVIDE;    break;
    case Qt::Key_Asterisk:  key = VK_MULTIPLY;  break;
    case Qt::Key_Period:    key = VK_DECIMAL;   break;
    default:                key = keyWithModifier & 0xFFFF;
    }

    in.type = INPUT_KEYBOARD;
    in.ki.wScan       = 0;
    in.ki.time        = 0;
    in.ki.dwExtraInfo = 0;


    // Press keys
    in.ki.dwFlags     = 0;

    if( withAlt )
    {
        in.ki.wVk = VK_MENU;
        SendInput(1, &in, sizeof(in));
    }
    if( withShift )
    {
        in.ki.wVk = VK_SHIFT;
        SendInput(1, &in, sizeof(in));
    }
    if( withControl )
    {
        in.ki.wVk = VK_CONTROL;
        SendInput(1, &in, sizeof(in));
    }

    in.ki.wVk         = key;
    SendInput(1, &in, sizeof(in));

    // And release keys
    in.ki.dwFlags     = KEYEVENTF_KEYUP;
    SendInput(1, &in, sizeof(in));

    if( withControl )
    {
        in.ki.wVk = VK_CONTROL;
        SendInput(1, &in, sizeof(in));
    }
    if( withShift )
    {
        in.ki.wVk = VK_SHIFT;
        SendInput(1, &in, sizeof(in));
    }
    if( withAlt )
    {
        in.ki.wVk = VK_MENU;
        SendInput(1, &in, sizeof(in));
    }

    return;
}



void sendWindowsKeyCombo(int keyWithModifier, bool withShift, bool withControl, bool withAlt)
{
    INPUT   in;
    WORD    key;

    key = keyWithModifier & 0xFFFF;

    in.type           = INPUT_KEYBOARD;
    in.ki.wScan       = 0;
    in.ki.time        = 0;
    in.ki.dwExtraInfo = 0;

    // Press keys
    in.ki.dwFlags     = 0;

    in.ki.wVk = VK_LWIN;
    SendInput(1, &in, sizeof(in));

    if( withAlt )
    {
        in.ki.wVk = VK_MENU;
        SendInput(1, &in, sizeof(in));
    }
    if( withShift )
    {
        in.ki.wVk = VK_SHIFT;
        SendInput(1, &in, sizeof(in));
    }
    if( withControl )
    {
        in.ki.wVk = VK_CONTROL;
        SendInput(1, &in, sizeof(in));
    }

    in.ki.wVk         = key;
    SendInput(1, &in, sizeof(in));

    // And release keys
    in.ki.dwFlags     = KEYEVENTF_KEYUP;
    SendInput(1, &in, sizeof(in));

    if( withControl )
    {
        in.ki.wVk = VK_CONTROL;
        SendInput(1, &in, sizeof(in));
    }
    if( withShift )
    {
        in.ki.wVk = VK_SHIFT;
        SendInput(1, &in, sizeof(in));
    }
    if( withAlt )
    {
        in.ki.wVk = VK_MENU;
        SendInput(1, &in, sizeof(in));
    }

    in.ki.wVk = VK_LWIN;
    SendInput(1, &in, sizeof(in));

    return;
}



void sendApplicationCommandOS(int applicationType, int command)
{
    qDebug("sendApplicationCommandOS(appTyle 0x%x, appCmmd 0x%x)",applicationType, command);

    switch( command )
    {
    case APP_CTRL_NO_ACTION:
        break;

    case APP_CTRL_START_PRESENTATION:

        if( applicationType == APP_POWERPOINT )
        {
            qDebug() << "PPT start: Send SHIFT-F5";
            sendKeypressToOS(VK_F5, true, false, false);    // [Shift-F5]
        }
        else if( applicationType == APP_PDF )
        {
            qDebug() << "PDF start: Send CTRL-L";
            sendKeypressToOS(0x4C, true, true, true);    // [CTRL-L]
        }
        break;

    case APP_CTRL_EXIT_PRESENTATION:

        if( applicationType == APP_POWERPOINT )
        {
            qDebug() << "PPT exit: Send Escape";
            sendKeypressToOS(VK_ESCAPE, false, false, false);    // [Escape]
        }
        else if( applicationType == APP_PDF )
        {
            qDebug() << "PDF exit: Send CTRL-L";
            sendKeypressToOS(0x4C, false, true, true);    // [CTRL-L]
        }
        break;

    case APP_CTRL_NEXT_PAGE:

        sendKeypressToOS(VK_SPACE, false, false, false);    // [Space bar]
        break;

    case APP_CTRL_PREV_PAGE:

        sendKeypressToOS(VK_PRIOR, false, false, false);    // [page up]
        break;

    case APP_CTRL_SHOW_HIGHLIGHT: // Handled in a drawing app, like freeStyleQt
        qDebug() << "Highlight toggle: no action.";
        break;

    default:
        qDebug() << "sendApplicationCommandOS() Unrecognised application command:" << command;
    }
}


void setMouseButtons(QPoint *loc, int buttons)
{
    static int old_buttons = 0;

    // The hardware doc states that the user interface messages are TBD. These
    // numbers correspond to the observed data that is returned from the board.
    if( buttons & BUTTON_LEFT_MOUSE )
    {
        if(( old_buttons & BUTTON_LEFT_MOUSE) == 0 )
        {
            mouse_event(MOUSEEVENTF_LEFTDOWN,  0,0, 0,0);
        }
        else if( buttons & BUTTON_TIP_NEAR_SURFACE ) // continuing: drag (but only LND is stable enough)
        {
            mouse_event(MOUSEEVENTF_MOVE,  0,0, 0,0);
        }
    }

    if( (old_buttons & BUTTON_LEFT_MOUSE) && (buttons & BUTTON_LEFT_MOUSE) == 0 )
    {
        mouse_event(MOUSEEVENTF_LEFTUP,  0,0, 0,0);
    }

    if( (buttons & BUTTON_MIDDLE_MOUSE) && (old_buttons & BUTTON_MIDDLE_MOUSE) == 0 )
    {
        mouse_event(MOUSEEVENTF_MIDDLEDOWN,  0,0, 0,0);
    }

    if( (old_buttons & BUTTON_MIDDLE_MOUSE) && (buttons & BUTTON_MIDDLE_MOUSE) == 0 )
    {
        mouse_event(MOUSEEVENTF_MIDDLEUP,  0,0, 0,0);
    }

    if( (buttons & BUTTON_RIGHT_MOUSE) && (old_buttons & BUTTON_RIGHT_MOUSE) == 0 )
    {
        mouse_event(MOUSEEVENTF_RIGHTDOWN,  0,0, 0,0);
    }

    if( (old_buttons & BUTTON_RIGHT_MOUSE) && (buttons & BUTTON_RIGHT_MOUSE) == 0 )
    {
        mouse_event(MOUSEEVENTF_RIGHTUP,  0,0, 0,0);
    }

    old_buttons = buttons;

    return;
}


#elif defined(Q_OS_LINUX)

bool    willAutoStart(void)
{
    return false;
}

void    setAutoStart(bool start)
{
}

void sendKeypressToOS(int keyWithModifier, bool withShift, bool withControl, bool withAlt)
{
    return;
}

void sendApplicationCommandOS(int applicationType, int command)
{
}

void setMouseButtons(QPoint *loc, int buttons)
{
    return;
}

#elif defined(__APPLE__) && defined(__MACH__)


void sendKeypressToOS(int keyWithModifier, bool withShift, bool withControl)
{
    return;
}

void setMouseButtons(QPoint *loc, int buttons)
{
    return;
}


#endif


#ifdef INVENSENSE_EV_ARM
invensenseReader  *InvSReader;
#endif


// A cross platform sleep class

class Sleeper : public QThread
{
public:
    static void wait_in_ms(unsigned long msecs) {QThread::msleep(msecs);}
};
void msleep(unsigned long t)
{
    Sleeper::wait_in_ms(t);
}

// Manage other applications

QProcess freeStyleProcess;

bool freeStyleIsRunning(void)
{
    return freeStyleProcess.state() == QProcess::Running;
}

void launchFreeStyle(void)
{
    freeStyleProcess.start("freeStyleQt");
}

void stopFreeStyle(void)
{
    if( freeStyleIsRunning() )
    {
        freeStyleProcess.terminate();
    }
}



// This is operating system specific

#ifdef Q_OS_WIN
QString getCosnetadevicePath(void)
{
    foreach( QFileInfo disk, QDir::drives() )
    {
        if( QFile::exists(disk.absolutePath() + "COSNETA/DEV/UPDATERD.DAT") &&
            QFile::exists(disk.absolutePath() + "COSNETA/DEV/UPDATEWR.DAT")    )
        {
            qDebug() << "Use dongle disk: " << disk.absolutePath();

            return disk.absolutePath();

            // NB For Flip, we also have COSNETA/DEV/SETTINGS0.DAT etc (if need to distinguish)
        }
    }
    return QString("dongle not found.");
}

QString getFreeRunnerPath(void)
{
    foreach( QFileInfo disk, QDir::drives() )
    {
        if( QFile::exists(disk.absolutePath() + "FRUN.DAT")    )
        {
            qDebug() << "Use FreeRunner disk: " << disk.absolutePath();

            return disk.absolutePath();
        }
    }
    return QString("dongle not found.");
}

#elif defined(Q_OS_LINUX)
#if 0
#define SEARCH_SCRIPT "cat /etc/mtab | grep  " \
                                      "`grep -l COSNETA /sys/block/sd?/device/vendor    | " \
                                      "sed 's/\\/sys\\/block\\//\\/dev\\//'             | " \
                                      "sed 's/\\/device\\/vendor//'` | awk '{print $2}'  "

QString getDonglePath(void)
{
    // getmntent() as drives() on Linux just returns "/"
    static char     *name         = NULL;
    static size_t    nch          = 0; 	// Size of allocated name buffer
    FILE            *read;
    int              chkPos;

    if( name && strlen(name)>= 4 ) // Assume min is /mnt
    {
        if( QFile::exists(name) )
        {
            return QString(name);
        }
    }

    read = popen(SEARCH_SCRIPT, "r");
    if( ! read ) return "NULL";

    if( getline(&name, &nch, read) < 4 )
    {
        fclose(read);
        return "NULL";
    }

    fclose(read);

    // Trim out any newline characters (and spaces) from the end

    chkPos = strlen(name) - 1;
    while( isspace(name[chkPos]) && chkPos >= 0 )
    {
        name[chkPos --] = (char)0;
    }

    qDebug() << "Dongle path: " << name;

    if( strlen(name) >= 4 ) return QString(name) + QDir::separator();

    return "NULL";
}
#endif

// Return a string representing the path to the root of the cosneta device.

QString getCosnetadevicePath(void)
{
    // Read the mtab, and choose between the available options there.
    QFile mountTab("/etc/mtab");

    if( ! mountTab.open(QIODevice::ReadOnly|QIODevice::Text) ) // Closed on leaving this method.
    {
        return QString("Failed to open mtab");
    }

    QTextStream reader(&mountTab);
    bool        found = false;

    QString line = reader.readLine();
    while( line.length() != NULL && ! found )
    {
        // Is it a dev line
        if( line.left(5) == QString("/dev/") )
        {
            // Get the count point from this line:
            QStringList list = line.split(" ",QString::SkipEmptyParts);

            if( list.size()>1 )
            {
                if( QFile::exists(list[1])                                          &&
                    QFile::exists(list[1]+QDir::separator()+"COSNETA")              &&
                    QFile::exists(list[1]+QDir::separator()+"COSNETA"+QDir::separator()+"DEV")    )
                {
                    // We've fopund a flip.
                    return list[1];
                }
            }
        }

        line = reader.readLine();
    }

    return QString("Flip not found.");
}

#endif


// Obfuscator for downloaded updates

quint32 XORPermutor = 0;

char nextXORByte(void)
{
    XORPermutor = 1 + 69069*XORPermutor;

    return XORPermutor & 255;
}



// /////////////////////////////////////////////////////////////////
// A bit of a nasty hack, but it really simplifies access in the GUI
// Please be very careful when accessing exposed interfaces here as
// they reside in different threads and so, there be monsters.
dongleReader      *dongle             = NULL;
CosnetaGui        *gui                = NULL;
networkReceiver   *networkInput       = NULL;
appComms          *Sender             = NULL;
savedSettings     *persistentSettings = NULL;
radarResponder    *radarEcho          = NULL;
QTime              globalTimer;


// Entry translation unit. Here we include the code for instantiating all the other code
// elements: the sysTray, the properties dialogue, the inter-task comms & the thread
// to read the data from the hardware device.

void balloonMessage(QString msg)
{
    if( gui != NULL )
    {
        gui->showBalloonMessage(msg,5);
    }
    else
    {
        QMessageBox::information(NULL, "Alert...", msg );
    }
}




// Manage updates

static QString updateDirName  = QDir::tempPath()  + QDir::separator() + QString("CosnetaUpdate");
#ifdef Q_OS_WIN
static QString updateFileName = updateDirName + QDir::separator() + QString("setup.exe");
#elif defined(Q_OS_LINUX)
static QString updateFileName = updateDirName + QDir::separator() + QString("setup");
#elif defined(Q_OS_MAC)
static QString updateFileName = updateDirName + QDir::separator() + QString("setup");
#endif

void clearUpdatePendingData(void)
{
    if( QFile::exists(updateDirName) )
    {
        QDir deletor(updateDirName);

        deletor.removeRecursively();
    }
}

// Called on program exit...
void checkForUpdatePending(void)
{
    if( QFile::exists(updateDirName) )
    {
        if( QFile::exists(updateDirName) )
        {
            QDesktopServices::openUrl(QString("file:%1").arg(updateFileName));
        }
        else
        {
            QMessageBox::warning(NULL,"Bad update data", "Incomplete update data found. Removing.");

            clearUpdatePendingData();
        }
    }
    else
    {
        // Clear any partial data lying around
        clearUpdatePendingData();
    }
}

// Remove any moved-aside files
void removeOldUpdateData(void)
{
    QDir         workingDir(QCoreApplication::applicationDirPath());
    QDirIterator iterator(workingDir,QDirIterator::Subdirectories);

    while( iterator.hasNext() )
    {
        QString filePath = iterator.next();

        if( ! iterator.fileInfo().isDir() )
        {
            // Don't copy the upgrade program.
            if( filePath.contains(QString(".OLD"),Qt::CaseInsensitive) )
            {
                QFile deletor(filePath);
                if( ! deletor.remove() ) qDebug() << "Failed to clean up:" << filePath;
            }
        }
    }
}




// Allow application exit to be called from anywhere

void quitProgram(void)
{
    gui->quitApp(false);
}




// //////////////
// Entry point...


int main(int argc, char *argv[])
{
    int                     ret;
    sysTray_devices_state_t sharedState;  // Shared between 3 threads: GUI, HW reader & sender
    QApplication            app(argc, argv);
#ifndef Q_OS_ANDROID

#if 0
    licenseManager          license;

    if( ! license.haveValidLicense() )
    {
        QWidget licenseInputDialogue = new QWidget(NULL, Qt::Dialog);

        QHBoxLayout *fullLayout = new QHBoxLayout;
        QVBoxLayout *dataLayout = new QVBoxLayout;
    }
#endif

#endif
#ifdef DEBUG_TAB
    qsrand(QTime::currentTime().msec());    // Ensure that various test random numbers are using a seeded random number
#endif
    
    // Load settings
    persistentSettings = new savedSettings();

    // Ensure that there isn't any update data hanging around from previous runs (or a hacker)
    clearUpdatePendingData();
    removeOldUpdateData();

    // Use a global timer for rate dependent calculations.
    globalTimer.start();

    // Initialise the devices table
    // the message_number & message_number_read belong to the appComms module.

    memset(&sharedState,0,sizeof(sysTray_devices_state_t));

    sharedState.applicationIsConnected = false;
    sharedState.leadPen                = 0;     // Gotta start somewhere
    sharedState.addRemoveDeviceMutex   = new QMutex();
    for(int p=0; p<MAX_PENS; p++)
    {
        COS_PEN_SET_ALL_FLAGS(sharedState.pen[p],COS_PEN_INACTIVE);
        sharedState.locCalc[p]              = new pen_location(&(sharedState.pen[p]),&(sharedState.settings[p]));
        sharedState.penMode[p]              = NORMAL_OVERLAY;
        sharedState.gestureDriveActive[p]   = false;
        sharedState.networkPenAddress[p][0] = (char)0;
    }

    // Start the process threads

    // Start the standard dongle data reader
#ifdef SUPPORT_USB_DEVICE
    QThread dongleThread;
    dongle = new dongleReader(&sharedState);

    dongle->doConnect(dongleThread);
    dongle->moveToThread(&dongleThread);
    dongleThread.start(QThread::HighestPriority);
#endif
    // Start read threads
#ifdef INVENSENSE_EV_ARM
    // Optional task to read data from an InvenSense ARM eval board. Allows multi-source
    QThread    InvSReaderThread;
    InvSReader = new invensenseReader(&sharedState);

    InvSReader->doConnect(InvSReaderThread);
    InvSReader->moveToThread(&InvSReaderThread);
    InvSReaderThread.start(QThread::NormalPriority);
#endif

    // Start a network receiver thread that allows, say, mobile phone apps data to be read

    QThread          networkReaderThread;
    networkReceiver *networkInput = new networkReceiver(&sharedState);

    networkInput->doConnect(networkReaderThread);
    networkInput->moveToThread(&networkReaderThread);
    networkReaderThread.start(QThread::NormalPriority);

    // Start the data send thread

    // Removed in Linux as not using this at the moment and the shared memory is often left over
    // after a crash, which is a pain to remove.

    QThread SenderThread;
    Sender  = new appComms(dongle, networkInput, &sharedState);

    Sender->doConnect(SenderThread);
    Sender->moveToThread(&SenderThread);
    SenderThread.start(QThread::NormalPriority);

    // Start the standard dongle data reader

    QThread radarEchoThread;
    radarEcho = new radarResponder(persistentSettings);

    radarEcho->doConnect(radarEchoThread);
    radarEcho->moveToThread(&radarEchoThread);
    radarEchoThread.start(QThread::LowPriority);

    // Ensure that the embedded images are initialised
    Q_INIT_RESOURCE(sysTrayQt);
    
    // Create a dialogue and the tray icon

    gui = new CosnetaGui(dongle, networkInput, &sharedState);
    gui->showInitialScreen();

    // GUI main loop

    ret = app.exec();

    // Stop the radar echo
    radarEcho->stopCommand();

    // Stop settings, writing current values to the system
    delete persistentSettings;

    // As this depends on our interface, stop it if it is running.
    stopFreeStyle();

    // Stop the process threads
    Sender->stopCommand();

#ifdef SUPPORT_USB_DEVICE
    dongle->stopCommand();
    dongleThread.wait(1000);   // Wait for up to 1 second
#endif
#ifndef Q_OS_LINUX
    SenderThread.wait(100);
#endif
    radarEchoThread.wait(500);

    qDebug() << "app->exec() returned" << ret;

    // Is there an update pending?
    checkForUpdatePending();

    return 0;
}

