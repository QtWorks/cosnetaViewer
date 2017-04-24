#include "overlaywindow.h"
#include <QApplication>
#include <QMessageBox>
#include <QPushButton>
#include <QQueue>

#ifdef Q_OS_LINUX
#include <signal.h>
#endif

#ifdef Q_OS_ANDROID
#include <QtAndroidExtras/QAndroidJniObject>
#include <QAndroidJniEnvironment>
#endif

#include <exception>      // std::set_terminate
#include <cstdlib>        // std::abort

#include <QDebug>

#include "build_options.h"

#include "handwritingRecognition.h"
#include "cosnetaAPI.h"
#include "debugLogger.h"


// Dummy stuff for Android. We don't use the API, and always report a failure to connect.
#ifdef TABLET_MODE

COS_BOOL cos_register_pen_event_callback(void (*callback)(pen_state_t *pen, int activePensMask), int maxRateHz, COS_BOOL onlyOnChanged) { return COS_FALSE; }
COS_BOOL cos_deregister_callback(void) { return COS_FALSE; }
COS_BOOL cos_get_pen_data(int penNum, pen_state_t* state) { return COS_FALSE; }
COS_BOOL cos_retreive_pen_settings(int pen_num, pen_settings_t *settings) { return COS_FALSE; }
COS_BOOL cos_update_pen_settings(int pen_num, pen_settings_t *settings) { return COS_FALSE; }
COS_BOOL cos_get_net_pen_addr(int penNum, char *address) { return COS_FALSE; }
int      cos_num_connected_pens(void) { return -1; }
COS_BOOL cos_API_is_running(void) { return COS_FALSE; }

#endif


#include "mainMenuDefinition.cpp"


#ifndef Q_OS_ANDROID
// Queue to receive requests for the slow handwriting recogniser thread
QQueue<hwrRequest_t> *requestQueue;
#endif
#ifdef Q_OS_WIN

#include <windows.h>

LONG WINAPI UnhandledException(LPEXCEPTION_POINTERS exceptionInfo)
{
    printf("An exception occurred which wasn't handled!\nCode: 0x%08X\nAddress: 0x%08X",
        exceptionInfo->ExceptionRecord->ExceptionCode,
        exceptionInfo->ExceptionRecord->ExceptionAddress);
    return EXCEPTION_EXECUTE_HANDLER;
}
#endif


#ifdef Q_OS_ANDROID

// Request multicast lock (required on some Android devices
void takeMulticastLock(void)
{
    qDebug() << "Acquire lock...";

    QAndroidJniObject::callStaticMethod<void>("com/cosneta/freeStyleQt/FreeStyleQt",
                                              "acquireWifiLock");
}

void releaseMulticastLock(void)
{
    qDebug() << "Release lock...";

    QAndroidJniObject::callStaticMethod<void>("com/cosneta/freeStyleQt/FreeStyleQt",
                                              "acquireWifiLock");
}

#endif



// Entry point...

int main(int argc, char *argv[])
{
#if defined(Q_OS_ANDROID) || defined(Q_OS_WIN)
//    initDebugLogger();
#endif

#ifdef Q_OS_WIN
    SetUnhandledExceptionFilter(UnhandledException);
#elif defined(Q_OS_ANDROID)
    init_crash_handler();
#endif
    QApplication    a(argc, argv);
    bool            runProgram = true;

#ifndef Q_OS_ANDROID
    bool            sysTrayIsRunning  = false;

    // start the thread to do the handwriting recognition
    requestQueue = new QQueue<hwrRequest_t>;

#ifdef HANDWRITING_RECOGNITION
    QThread hwrThread;
    handwritingRecognition hwrRecogniser;

    hwrRecogniser.doConnect(hwrThread);
    hwrRecogniser.moveToThread(&hwrThread);
    hwrThread.setObjectName("HandwritingRecognition");
    hwrThread.start(QThread::NormalPriority);
#endif
    // Try to use the cosnetaAPI, but if not there, default start in mouse mode.
    if( cos_API_is_running() == COS_TRUE )
    {
        sysTrayIsRunning  = true;
    }
#else
    bool            sysTrayIsRunning  = false;
#endif
#ifdef Q_OS_LINUX
    signal(SIGPIPE, SIG_IGN);  // Don't abort on a remote socket disconnecting...
#endif

    int ret = 0;

    if( runProgram )
    {
        qDebug() << "call createPenMenuData()";

        createPenMenuData();

#ifdef Q_OS_ANDROID
        takeMulticastLock();
#endif
        overlayWindow w(sysTrayIsRunning);

//#if defined(Q_OS_MAC) || defined(Q_OS_ANDROID)
//        w.show();
//#else
//        w.showFullScreen();
//#endif
        ret =  a.exec();

#ifdef Q_OS_ANDROID
        releaseMulticastLock();
#endif
        qDebug() << "Main window exited with a return code of" << ret;
    }

#ifndef Q_OS_ANDROID

#ifdef HANDWRITING_RECOGNITION
    hwrRecogniser.stopCommand();
    if( ! hwrThread.wait(500) )
    {
        qDebug() << "Handwriting thread stop timed out. Terminate!";

        hwrThread.terminate();
    }

    delete requestQueue;
#endif
#endif

#if defined(Q_OS_WIN) || defined(Q_OS_ANDROID)
    closeAndFlushDebugFiler();
#endif
    return ret;
}
