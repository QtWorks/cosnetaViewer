// (c) Cosneta Ltd.
//
// This module manages the GUI display of the overlay, cursors and menus.
// Additionally, it polls the cosnetaAPI for user inputs and drives the
// overlay.

//ln 1810

#include <QTimer>
#include <QPainter>
#include <QPaintEvent>
#include <QFileDialog>
#include <QMessageBox>
#include <QApplication>
#include <QDesktopWidget>
#include <QDesktopServices>
#include <QBuffer>
#include <QApplication>
#include <QGridLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSystemTrayIcon>

#include <QDebug>

// Definition of the menu
#include "penStates.h"

#include "overlaywindow.h"
#include "overlay.h"
#include "textOverlay.h"
#include "menuStructure.h"
#include "remoteConnectDialogue.h"
#include "debugLogger.h"
#include "cosnetaAPI.h"
#include "../systrayqt/net_connections.h"


#include <iostream>

//#define DEBUG 1

#ifdef DEBUG
#include <QMessageBox>
#endif

#ifdef TABLET_MODE
overlayWindow::overlayWindow(bool sysTrayIsRunning, QMainWindow *parent) : QMainWindow(parent)
#else
overlayWindow::overlayWindow(bool sysTrayIsRunning, QWidget *parent) :
#ifndef Q_OS_LINUX
                     QWidget(parent, Qt::FramelessWindowHint        |
                                     Qt::X11BypassWindowManagerHint |
                                     Qt::WindowStaysOnTopHint        )
#else
                     QWidget(parent )
#endif
#endif
{
    if( ! QSystemTrayIcon::isSystemTrayAvailable() )
    {
#ifndef TABLET_MODE
        QMessageBox::critical(0, QObject::tr("Cosneta FreeStyle"),
                              QObject::tr("No system tray icon available, so only pen controls available.") );

        qDebug() << "No systray icon possible.";
#endif
        trayIconPresent = false;
    }
#ifndef TABLET_MODE
    else
    {
        // load icon
#ifdef Q_OS_WIN
        QIcon icon = QIcon(":/images/freestyle_logo.png");           // Works on windows
#elif defined(Q_OS_MAC)
        QIcon icon = QIcon(":/images/freestyle_logo_vector_bw.svg"); // Want monochrome icon on Mac
#else // Q_OS_UNIX
        QIcon icon = QIcon(":/images/freestyle_logo_vector.svg");    // Tested on Kubuntu
#endif
        setWindowIcon(icon);

        // Menu actions
        trayIconMenu = new QMenu(this);

        localMouseAction = new QAction(tr("Local &Mouse as Pen"), this);
        localMouseAction->setCheckable(true);
        localMouseAction->setChecked(false);
        connect(localMouseAction, SIGNAL(triggered()), this, SLOT(toggleMouseAsPen()));
        trayIconMenu->addAction(localMouseAction);

        // Available speed list
        if( sysTrayIsRunning )
        {
            this->setWindowTitle("Freestyle Host");
            QMenu *dataRateMenu = trayIconMenu->addMenu(tr("Bandwidth"));
            rate250 = new QAction(tr("250 Kbit/s"),this);
            rate250->setCheckable(true);
            rate250->setChecked(false);
            connect(rate250, SIGNAL(triggered(bool)), this, SLOT(setRate250()));
            rate500 = new QAction(tr("500 Kbit/s"),this);
            rate500->setCheckable(true);
            rate500->setChecked(true);
            lastAction = rate500;
            connect(rate500, SIGNAL(triggered(bool)), this, SLOT(setRate500()));
            rate750 = new QAction(tr("750 Kbit/s"),this);
            rate750->setCheckable(true);
            rate750->setChecked(false);
            connect(rate750, SIGNAL(triggered(bool)), this, SLOT(setRate750()));
            rate1000 = new QAction(tr("1.0 Mbit/s"),this);
            rate1000->setCheckable(true);
            rate1000->setChecked(false);
            connect(rate1000, SIGNAL(triggered(bool)), this, SLOT(setRate1000()));
            rate1500 = new QAction(tr("1.5 Mbit/s"),this);
            rate1500->setCheckable(true);
            rate1500->setChecked(false);
            connect(rate1500, SIGNAL(triggered(bool)), this, SLOT(setRate1500()));
            rate2000 = new QAction(tr("2.0 Mbit/s"),this);
            rate2000->setCheckable(true);
            rate2000->setChecked(false);
            connect(rate2000, SIGNAL(triggered(bool)), this, SLOT(setRate2000()));
            rate4000 = new QAction(tr("4.0 Mbit/s"),this);
            rate4000->setCheckable(true);
            rate4000->setChecked(false);
            connect(rate4000, SIGNAL(triggered(bool)), this, SLOT(setRate4000()));
            rate6000 = new QAction(tr("6.0 Mbit/s"),this);
            rate6000->setCheckable(true);
            rate6000->setChecked(false);
            connect(rate6000, SIGNAL(triggered(bool)), this, SLOT(setRate6000()));
            dataRateMenu->addAction(rate250);
            dataRateMenu->addAction(rate500);
            dataRateMenu->addAction(rate750);
            dataRateMenu->addAction(rate1000);
            dataRateMenu->addAction(rate1500);
            dataRateMenu->addAction(rate2000);
            dataRateMenu->addAction(rate4000);
            dataRateMenu->addAction(rate6000);
        }

        helpAction = new QAction(tr("&Help"), this);
        connect(helpAction, SIGNAL(triggered(bool)), this, SLOT(help()));
        trayIconMenu->addAction(helpAction);

        trayIconMenu->addSeparator();

        quitAction = new QAction(tr("&Quit"), this);
        connect(quitAction, SIGNAL(triggered(bool)), this, SLOT(mainMenuQuit()));
        trayIconMenu->addAction(quitAction);

        // set up and show the system tray icon
        trayIcon = new QSystemTrayIcon(icon,this);
        trayIcon->setContextMenu(trayIconMenu);
        trayIcon->setToolTip(tr("Freestyle Host"));
        trayIcon->show();

        connect(trayIcon,SIGNAL(activated(QSystemTrayIcon::ActivationReason)),
                this,    SLOT(trayIconClicked(QSystemTrayIcon::ActivationReason)));

        trayIconPresent = true;
    }
#endif
    desktop = new QDesktopWidget;

    // Function differences here: cannot have transparent window,
    //  but mouse functions as per guiSender.
    useMouseAsInputToo = ! sysTrayIsRunning;
    sysTrayDetected    = sysTrayIsRunning;

    // Flags used in repaint: don't do it until we're ready
    showActionImage        = false;
    pageSelectActive       = false;
    playingRecordedSession = false;
#ifdef TABLET_MODE
    showIntroBackground    = true;
#else
    showIntroBackground    = false;
#endif

    // TODO: move this to main.cpp as we're moving static data there
    defaultPenColour[0] = Qt::red;        defaultPenColour[1] = Qt::green;
    defaultPenColour[2] = Qt::yellow;     defaultPenColour[3] = Qt::blue;
    defaultPenColour[4] = Qt::magenta;    defaultPenColour[5] = Qt::cyan;
    defaultPenColour[6] = Qt::black;      defaultPenColour[7] = Qt::white;
    defaultPenColour[8] = Qt::lightGray;

    localPenColour = Qt::magenta;

    defaultOpaqueBackgroundColour = Qt::darkGray;

    width  = desktop->width();
    height = desktop->height();

//    width  = desktop->availableGeometry().width();
//    height = desktop->availableGeometry().height();

    qDebug() << "Available geometry:" << width << height;

    // For remote/replay views to allow scaling/zooming in
    nonLocalViewScale    = 1.0;
    nonLocalViewScaleMax = 1.0;
    nonLocalViewCentreX  = width/2;
    nonLocalViewCentreY  = height/2;
    inPinchZoom          = false;
    inTouchDrag          = false;

    // Store a pointer to the the annotations (the overlay)
    // TODO: remove objects unused in Android (we need the space)

    annotationsImage      = new QImage(width, height, QImage::Format_ARGB32_Premultiplied);
    highlights            = new QImage(width, height, QImage::Format_ARGB32_Premultiplied);
    replayBackgroundImage = new QImage(width, height, QImage::Format_ARGB32_Premultiplied);
    textLayerImage        = new QImage(width, height, QImage::Format_ARGB32_Premultiplied);

    if( annotationsImage == NULL      || highlights == NULL ||
        replayBackgroundImage == NULL || textLayerImage == NULL )
    {
        QMessageBox::critical(NULL,tr("Memory failure"),tr("Failed to allocate screen image data."));
    }

    annotationsImage->fill(Qt::transparent);
    highlights->fill(Qt::transparent);
    replayBackgroundImage->fill(Qt::transparent);
    textLayerImage->fill(Qt::transparent);

    // Create a file dialogue that will be under the pens
    fileDialogue = new QFileDialog(this);
    connect(fileDialogue, SIGNAL(accepted()), this, SLOT(fileDialogAccept()) );
    connect(fileDialogue, SIGNAL(rejected()), this, SLOT(fileDialogReject()) );
    fileDialogue->hide();
    fileDialogue->setDirectory(QDir::homePath());
    fileDialogue->setOption(QFileDialog::DontConfirmOverwrite); // Y/N needs mouse & stops main loop
    fileDialogueAction = FD_NO_ACTION;

    remoteSelector = NULL;

    // Create per pen data
    for(int penNum=0; penNum<MAX_SCREEN_PENS; penNum++)
    {
        // The following widgets initialise to hidden
        cursor[penNum]       = new penCursor(defaultPenColour[penNum],this);
        dialogCursor[penNum] = new penCursor(defaultPenColour[penNum],fileDialogue);

        brushAction[penNum]                = false;
        penWasActive[penNum]               = false;
        penLastMode[penNum]                = COS_MODE_NORMAL;
        waitForPenButtonsRelease[penNum]   = false;

        penInTextMode[penNum]              = false;

        addingNewStickyMode[penNum]        = false;
        deleteStickyOnClick[penNum]        = false;
        startMoveOnStickyClick[penNum]     = false;
        penIsDraggingSticky[penNum]        = false;
        stickyBeingUpdatedByPenNum[penNum] = NULL;
        penStickyNumber[penNum]            = INVALID_STICKY_NUM;
    }

    QColor defBackCol;
    defBackCol = sysTrayIsRunning ? Qt::darkGray : Qt::transparent;

#ifndef TABLET_MODE
    // Not on tablets. These are client only, so we don't need these functions.
    screenGrab = new screenGrabber(this);

    screenGrab->backgroundIsOpaque = useMouseAsInputToo;
//    screenGrab->start();
    if( sysTrayIsRunning )
        screenGrab->start();

    broadcast = new viewBroadcaster(screenGrab,penWasActive,this);
//    broadcast->start();
    if( sysTrayIsRunning )
    {
        qDebug() << "Client mode: pauseSending";
        broadcast->start();
    }
#endif

    remoteJoinSkt.bind(14758); // Listen here for accepted by sysTray response   QUdpSocket::ShareAddress);
    qDebug() << "overlayWindow: bind() remoteSkt" << remoteJoinSkt.socketDescriptor() << "to 14758";
//    qDebug() << "overlayWindow: bind() remoteBroadcastSkt to" << BROADCAST_VIEW_PORT;
//    remoteBroadcastSkt.bind(BROADCAST_VIEW_PORT); // ,QUdpSocket::ShareAddress);

    annotations = new overlay(      annotationsImage,highlights,textLayerImage, defaultPenColour, defBackCol, true, screenGrab, this);
    replay      = new replayManager(annotationsImage,highlights,textLayerImage, defaultPenColour, this);  // Both draw to same image...
    netClient   = new networkClient(annotationsImage,highlights,textLayerImage, cursor, this);
#ifdef USE_BUTTON_STRIP
    menuButtons = new buttonStrip(LOCAL_MOUSE_PEN,this);
#endif

#ifdef USE_THREE_BUTTON_UI
    threeButtonInterface = new threeButtonUI(LOCAL_MOUSE_PEN, width>1024,this);
    threeButtonInterface->show(); // DEVELOPMENT ONLY
    threeButtonInterface->raise();
#endif
    drawTrace      = new localDrawTrace(annotationsImage,this);
    //pNoteDrawTrace = new privateNoteDrawTrace(annotationsImage, this);       //pn test

    netClient->doConnect(viewReceiverThread);
    netClient->moveToThread(&viewReceiverThread);
    viewReceiverThread.setObjectName("networkClient");
    viewReceiverThread.start();

    connect(netClient, SIGNAL(resolutionChanged(int, int)), this, SLOT(remoteResolutionChanged(int, int)));
    connect(netClient, SIGNAL(needsRepaint()),              this, SLOT(netClientUpdate()));
    connect(netClient, SIGNAL(remoteViewDisconnected()),    this, SLOT(mainMenuDisconnect()));
    connect(netClient, SIGNAL(remotePenDisconnected()),     this, SLOT(penDisconnectedOnHost()));
    connect(netClient, SIGNAL(remoteViewAvailable()),       this, SLOT(remoteViewCanBeShown()));
//    connect(netClient, SIGNAL(moveReceivedPen(int penNum, int x, int y, bool show)),
//            this,      SLOT(  movePenFromReceivedData(int penNum, int x, int y, bool show)));

    // Create per pen data that needs reference to the above classes
    for(int penNum=0; penNum<MAX_SCREEN_PENS; penNum++)
    {
        penMenu[penNum] = new menuStructure(this,penNum,cursor[penNum],annotations,replay);

        // Initial state
        penMenu[penNum]->setColour(defaultPenColour[penNum]);
        penMenu[penNum]->setThickness(8);
    }

    pagePreview.clear();
    previewWidth      = width / 30;
    currentPageNumber = 0;
//    QImage first(previewWidth,height/30, QImage::Format_ARGB32_Premultiplied);
    pagePreview.push_back( annotationsImage->scaledToWidth(previewWidth) );

    // Have a small logo so we know it's running
    titleImage      = imageOfDrawnText(tr("Idle"));
    showActionImage = false;

    // This next causes the cursors to leave a trail...
    // make it transparent
#ifdef Q_OS_MAC
//    setAttribute(Qt::WA_OpaquePaintEvent,true);
//    setAttribute(Qt::WA_NoSystemBackground,true);
//    setAttribute(Qt::WA_MacNoShadow,true);
//    setAttribute(Qt::WA_MacShowFocusRect,false);
#endif
#ifdef TABLET_MODE
    displayMenuButtonStrip = true;
    localControlIsZoomPan  = true;
    setAttribute(Qt::WA_AcceptTouchEvents);
#else
    displayMenuButtonStrip = false;
    localControlIsZoomPan  = false;
    setAttribute(Qt::WA_TranslucentBackground,true);     // causes WA_NoSystemBackground to be set
#ifdef Q_OS_LINUX
    setAttribute(Qt::WA_TransparentForMouseEvents,true); // Pass mouse events through
#endif
//    setAttribute(Qt::WA_OpaquePaintEvent);               // We update everything on repaint: don't clear first
#endif

    remoteConnection       = false;
    netClientReceivedWords = 0;
    lastAckedReceivedWords = 0;

    wheelDeltaAccumulate           = 0;
    mouseButtons                   = 0;
    keypressSequenceNumber         = 0;
    lastKeypressSequenceNumber     = keypressSequenceNumber;
    menuSequenceNumber             = 0;
    lastMenuSequenceNumber         = menuSequenceNumber;
    presentationSequenceNumber     = 0;
    lastPresentationSequenceNumber = presentationSequenceNumber;
    sendSequenceNumber             = 0;
    currentSendAppCommand          = 0;
    mouseButtonsPulse              = 0;
    mouseButtonsPulseCount         = 0;
    modeSendRepeat                 = 0;
    mouseX                         = 0;
    mouseY                         = 0;

    setMouseTracking(true);

    if( useMouseAsInputToo )
    {
        sessionType = PAGE_TYPE_WHITEBOARD;

        setCursor(Qt::BlankCursor);
        annotations->setBackground(defaultOpaqueBackgroundColour);
        replayBackgroundImage->fill(defaultOpaqueBackgroundColour);

        // Give the mouse a initial state, as no events will have happened before first update
        mouseX       = QCursor::pos().x();
        mouseY       = QCursor::pos().y();
    }
    else
    {
        sessionType = PAGE_TYPE_OVERLAY;

        annotations->setBackground(Qt::transparent);
        replayBackgroundImage->fill(Qt::transparent);
    }

    // Title position (will be selectable later). Shows when in playback/record modes.
    titlePos = QPoint(10,25);    // Moved down from top edge for MAC menubars

    // Jog shuttle
    jogShuttleImage = imageOfJogShuttle(width,true);
    jogShuttlePos   = QRect( (width-jogShuttleImage->width())/2,height*9/10,
                             jogShuttleImage->width(),jogShuttleImage->height() );

    // Initialise the "local mouse" pen (could be enabled by a user option later
    cursor[LOCAL_MOUSE_PEN]->updatePenColour(Qt::magenta);
    penMenu[LOCAL_MOUSE_PEN]->setColour(Qt::magenta);
    annotations->setPenColour(LOCAL_MOUSE_PEN,Qt::magenta);
    replay->setPenColour(LOCAL_MOUSE_PEN,Qt::magenta);
    strncpy(penSettings[LOCAL_MOUSE_PEN].users_name,
                       (QHostInfo::localHostName()+tr(" controller")).toLocal8Bit(),
                        MAX_USER_NAME_SZ);
    penSettings[LOCAL_MOUSE_PEN].users_name[MAX_USER_NAME_SZ-1] = (char)0;

    // NB We were show in main (Qt5 doesn't seem to like a re-show as it then renders nothing)

    playingRecordedSession = false;
    inHostedMode           = false;

    connectedToViewHost    = false;
#ifndef TABLET_MODE
#ifndef QT_DEBUG
    //showFullScreen(); // Replaced by the following for Qt5.2.1
    resize(width,height);  // Do not change to (w,h) until overlay clear to mouse events
    move(0,0);
#else
    resize(width*2/3,height*2/3);  // Do not change to (w,h) until overlay clear to mouse events
    move(width/6,height/6);
#endif
#endif
    show();

    // Draw once
    repaint();
#ifndef Q_OS_LINUX
    activateWindow();
#endif
#ifdef Q_OS_MAC
    activateWindow();
    qDebug()<< "Window active:"<< isActiveWindow();
#endif

    // Register callbacks to detect display changes
    numScreens = desktop->screenCount();
//    connect(desktop, SIGNAL(resized(int)), this, SLOT(screenResized(int)) );
//    connect(desktop, SIGNAL(screenCountChanged(int)), this, SLOT(numScreensChanged(int)) );

    if( numScreens != 1 )
    {
//        chooseScreenForOverlay();
    }

#ifdef TABLET_MODE
    // Kick off the connect to remote host dialogue as this is always a remote session
    remoteNetSelectAndConnect(LOCAL_MOUSE_PEN);
    playbackRefire     = NULL;
    lastSendCharRepeat = 0;

#ifdef USE_BUTTON_STRIP
    menuButtons->screenResized(width,height); // NB No account of buttons in width here as not visible yet.
#endif
#ifdef USE_THREE_BUTTON_UI
    threeButtonInterface->screenResized(width,height);
#endif

//    textInputDialogue = NULL;
    createTextInputWindow();

#ifdef USE_THREE_BUTTON_UI
    threeButtonInterface->screenResized(width,height);
    threeButtonInterface->hide();
#endif

#ifdef USE_BUTTON_STRIP
    // TODO: fix menu buttons so we can use them on the host too
    qDebug() << "Hide menubuttons";
    menuButtons->hide();
#endif
#else
    // Create a timer to do oneshot timeouts to update playback
    playbackRefire = new QTimer(this);

#ifdef USE_THREE_BUTTON_UI
    threeButtonInterface->screenResized(width,height);
#endif

#ifdef USE_BUTTON_STRIP
    // TODO: fix menu buttons so we can use them on the host too
    qDebug() << "Hide menubuttons";
#endif
#endif

    // Create a repeating timer in the GUI thread to poll the mouse pointers
    timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), this, SLOT(readPenData()));
    timer->start(20);   // 20mS, ie 50Hz

    qDebug() << "overlayWindow created.";
}

overlayWindow::~overlayWindow()
{
    qDebug() << "Stop timer...";
    timer->stop();

    qDebug() << "Stop screenGrabber and netClient...";
    netClient->haltNetworkClient();
#ifndef TABLET_MODE
    screenGrab->terminate();
    if( ! screenGrab->wait(200) ) qDebug() << "Terminate of screenGrab timed out.";
#endif
    if( ! viewReceiverThread.wait(400) )
    {
        qDebug() << "stopCommand on viewReceiverThread timed out.";
        viewReceiverThread.terminate();
        viewReceiverThread.wait(400);
    }

    for(int penNum=0; penNum<MAX_SCREEN_PENS; penNum++)
    {
        penMenu[penNum]->hide();
        delete penMenu[penNum];
        cursor[penNum]->hide();
        delete cursor[penNum];
    }

    qDebug() << "Hide and exit...";
    hide();

    if( remoteSelector ) remoteSelector->hide();
#ifdef TABLET_MODE
    textInputDialogue->hide();
#endif
    // TODO: Android object removal
    delete annotationsImage;
    delete highlights;
    delete replayBackgroundImage;
    delete textLayerImage;
    delete desktop;
    delete fileDialogue;
    delete titleImage;
}

void overlayWindow::trayIconClicked(QSystemTrayIcon::ActivationReason reason)
{
    qDebug() << "trayIconClicked. Reason:" << reason;
#ifndef TABLET_MODE
    /*trayIcon->showMessage("Data rate",
                          QString("freeStyle %1 Mbs").arg(broadcast->getCurrentSendRateMbs()),
                          QSystemTrayIcon::Information, 5000);*/
#endif
}

void overlayWindow::createTextInputWindow(void)
{
    // Create the widgets & layout
    textInputDialogue = new QWidget(this, Qt::Dialog);

    arbritraryTextInput = new QLineEdit;
    QPushButton *sendTextBtn  = new QPushButton(tr("Send"));
    QPushButton *returnKeyBtn = new QPushButton(tr("Ret"));
    QPushButton *backKeyBtn   = new QPushButton(tr("<Del"));

    QHBoxLayout *layout = new QHBoxLayout;
    layout->addWidget(arbritraryTextInput);
    layout->addWidget(sendTextBtn);
    layout->addWidget(returnKeyBtn);
    layout->addWidget(backKeyBtn);

    // And let's do stuff with the text and buttons
    connect(sendTextBtn,  SIGNAL(clicked(bool)), this, SLOT(textInputSend(bool)) );
    connect(returnKeyBtn, SIGNAL(clicked(bool)), this, SLOT(textInputReturnPressed(bool)) );
    connect(backKeyBtn,   SIGNAL(clicked(bool)), this, SLOT(textInputDeletePressed(bool)) );

    textInputDialogue->setLayout(layout);

    arbritraryTextInput->setFocus();

    // Starts hidden
    textInputDialogue->hide();
}

void overlayWindow::textInputSend(bool checked)
{
    qDebug() << "textInputSend()" << arbritraryTextInput->text();

    QString shiftedSymbols = "!\"£$%^&*()_+{}:@~<>?|";
    QString equivToSymbols = "1234567890-=[];'#,./\\";

    QString sendString = arbritraryTextInput->text();
    arbritraryTextInput->setText("");

    int  ch;
    int  mods;

    for(int chnum=0; chnum<sendString.length(); chnum++ )
    {
        ch = sendString[chnum].cell();

        // Suitable for: if( ch >= 'A' && ch 'Z' ),
        //               if( ch >= '1' && ch '0' ),
        //               equivToSymbols()
        mods = 0;

        if( ch >= 'a' && ch <= 'z' )
        {
            ch   = ch - 0x20;
            mods = 0;
        }
        else if( ch >= 'A' && ch <= 'Z' )
        {
            mods = Qt::ShiftModifier;
        }
        else
        {
            int idx = shiftedSymbols.indexOf(sendString[chnum]);

            if( idx >= 0 )
            {
                mods = Qt::ShiftModifier;
                ch   = equivToSymbols[idx].cell();
            }
        }
        sendCharacters.append( ch | mods );
    }

    qDebug() << "SendCharacters queue size:" << sendCharacters.size();
}

void overlayWindow::textInputReturnPressed(bool checked)
{
//    qDebug() << "textInputReturnPressed()";

    sendCharacters.append( Qt::Key_Return );
}

void overlayWindow::textInputDeletePressed(bool checked)
{
//    qDebug() << "textInputDeletePressed()";

    sendCharacters.append( Qt::Key_Backspace );
}


void overlayWindow::enableTextInputWindow(bool on)
{
    if( on )
    {
        if( textInputDialogue == NULL ) createTextInputWindow();

        textInputDialogue->show();
        textInputDialogue->move(30,height-30-textInputDialogue->height());
        textInputDialogue->activateWindow();
        textInputDialogue->raise();

        qDebug() << "Show text input dialogue. Parent height =" << height
                 << "dialogue height =" << textInputDialogue->height();
    }
    else
    {
        qDebug() << "Hide text input dialogue.";

        if( textInputDialogue ) textInputDialogue->hide();
    }
}


#ifdef TABLET_MODE
void overlayWindow::setRate250()  {  }
void overlayWindow::setRate500()  {  }
void overlayWindow::setRate750()  {  }
void overlayWindow::setRate1000() {  }
void overlayWindow::setRate1500() {  }
void overlayWindow::setRate2000() {  }
void overlayWindow::setRate4000() {  }
void overlayWindow::setRate6000() {  }
#else
void overlayWindow::setRate250()  { broadcast->setAverageDataRate( 250000); toggleNewRate( rate250); }
void overlayWindow::setRate500()  { broadcast->setAverageDataRate( 500000); toggleNewRate( rate500); }
void overlayWindow::setRate750()  { broadcast->setAverageDataRate( 750000); toggleNewRate( rate750); }
void overlayWindow::setRate1000() { broadcast->setAverageDataRate(1000000); toggleNewRate(rate1000); }
void overlayWindow::setRate1500() { broadcast->setAverageDataRate(1500000); toggleNewRate(rate1500); }
void overlayWindow::setRate2000() { broadcast->setAverageDataRate(2000000); toggleNewRate(rate2000); }
void overlayWindow::setRate4000() { broadcast->setAverageDataRate(4000000); toggleNewRate(rate4000); }
void overlayWindow::setRate6000() { broadcast->setAverageDataRate(6000000); toggleNewRate(rate6000); }
#endif

void overlayWindow::toggleNewRate(QAction *newAction)
{
    lastAction->setChecked(false);
    newAction->setChecked(true);
    lastAction = newAction;
}


void overlayWindow::help()
{
#ifdef TABLET_MODE
    QMessageBox::warning(this,tr("No help"),tr("Help window for tablet not written yet. TODO."));
#else
    QString helpFileName = QString("help") + QDir::separator() + QString("freestyle_index.html");
    QDesktopServices::openUrl(QString("file:%1").arg(helpFileName));
#endif
}

// And used elsewhere
void overlayWindow::resetPens(void)
{
    // State variables

    for(int penNum=0; penNum<MAX_SCREEN_PENS; penNum++)
    {
        penIsDrawing[penNum]   = false;

        penMenu[penNum]->setColour(defaultPenColour[penNum]);
        penMenu[penNum]->setThickness(8);
        penMenu[penNum]->setDrawStyle(DRAW_SOLID);

#if 0    // Handle these in menuStructure now
        penColourChanged(penNum, penNum);
        penThicknessChanged(8, penNum);
        penDrawTypeChanged(ACTION_DRAW, penNum);
#endif
    }
}


// ////////
// Slots...
// ////////

// Pen menus //

// ///////// //
// Host menu //
// ///////// //


void overlayWindow::newPage(void)
{
    if( remoteConnection )
    {
        menuCommand = COMMAND_TO_TOP_BYTE(PAGE_ACTION) | PAGE_NEW;
        menuSequenceNumber ++;

        return;
    }

    // Store/update thumbnail of current page
    if( currentPageNumber >= ((int)(pagePreview.size())) )
    {
        pagePreview.push_back( annotationsImage->scaledToWidth(previewWidth) );
    }
    else
    {
        pagePreview[currentPageNumber] = annotationsImage->scaledToWidth(previewWidth);
    }

    // Go to last page
    annotations->gotoPage(pagePreview.size() - 1);
    replay->gotoPage(pagePreview.size() - 1);

    // add a new page
    currentPageNumber = pagePreview.size();
    annotations->nextPage();
    replay->nextPage();
}



void overlayWindow::nextPage(void)
{
    if( remoteConnection )
    {
        menuCommand = COMMAND_TO_TOP_BYTE(PAGE_ACTION) | PAGE_NEXT;
        menuSequenceNumber ++;

        return;
    }

    qDebug() << "overlayWindow::nextPage()";

    // Store/update thumbnail of current page
    if( currentPageNumber >= ((int)(pagePreview.size())) )
    {
        pagePreview.push_back( annotationsImage->scaledToWidth(previewWidth) );
    }
    else
    {
        pagePreview[currentPageNumber] = annotationsImage->scaledToWidth(previewWidth);
    }
    localScreenSnap();
    // Go to next page
    currentPageNumber ++;
    annotations->nextPage();
    replay->nextPage();
}


void overlayWindow::prevPage(void)
{
    if( remoteConnection )
    {
        menuCommand = COMMAND_TO_TOP_BYTE(PAGE_ACTION) | PAGE_PREV;
        menuSequenceNumber ++;

        return;
    }

    if( currentPageNumber>0 )
    {
        if( currentPageNumber >= ((int)(pagePreview.size())) )
        {
            pagePreview.push_back( annotationsImage->scaledToWidth(previewWidth) );
        }
        else
        {
            pagePreview[currentPageNumber] = annotationsImage->scaledToWidth(previewWidth);
        }
        annotations->previousPage();
        replay->previousPage();

        currentPageNumber --;
    }
}


void overlayWindow::showPageSelector(void)
{
    if( remoteConnection )
    {
        menuCommand = COMMAND_TO_TOP_BYTE(PAGE_ACTION) | PAGE_SELECTOR;
        menuSequenceNumber ++;

        return;
    }

    firstShownThumbNail = 0;

    pageSelectImage  = imageOfPageSelect();
    pageSelectActive = true;

    update();
}


void overlayWindow::clearOverlay(void)
{
    if( remoteConnection )
    {
        menuCommand = COMMAND_TO_TOP_BYTE(PAGE_ACTION) | PAGE_CLEAR;
        menuSequenceNumber ++;

        return;
    }

    annotations->clearOverlay();
    replay->clearOverlay();
}

void overlayWindow::clearPrivateNotes(void)
{
    drawTrace->resetPrivateDrawCache();
}


void overlayWindow::setTextPosFromPixelPos(int penNum, int x, int y)
{
    if( sessionType == PAGE_TYPE_STICKIES )
    {
        if( stickyBeingUpdatedByPenNum[penNum] != NULL )
        {
            if( stickyBeingUpdatedByPenNum[penNum]->screenRect().contains(QPoint(x,y)) )
            {
                int snX = stickyBeingUpdatedByPenNum[penNum]->x();
                int snY = stickyBeingUpdatedByPenNum[penNum]->y();

                qDebug() << "Stick note pen pos: screen coords" << QPoint(x,y)
                         << "Note coords" << QPoint(snX,snY);

                stickyBeingUpdatedByPenNum[penNum]->setTextPosFromPixelPos(penNum, x-snX, y-snY);
            }
            else
            {
                // Was a click outside the note: deselect note
                stickyBeingUpdatedByPenNum[penNum] = NULL;
            }
        }
    }
    else
    {
        // Add to the main overlay/whiteboard
        annotations->setTextPosFromPixelPos(penNum, x, y);
        replay->setTextPosFromPixelPos(penNum, x, y);
    }
}

void overlayWindow::addTextCharacterToOverlay(int penNum, quint32 charData)
{
    if( APP_CTRL_IS_RAW_KEYPRESS(charData) )
    {
        if( sessionType != PAGE_TYPE_STICKIES )
        {
            // Update the main overlay/whiteboard
            annotations->addTextCharacterToOverlay(penNum, APP_CTRL_MODKEY_FROM_RAW_KEYPRESS(charData));
            replay->addTextCharacterToOverlay(penNum, APP_CTRL_MODKEY_FROM_RAW_KEYPRESS(charData));
        }
        else
        {
            if( stickyBeingUpdatedByPenNum[penNum] != NULL )
            {
                stickyBeingUpdatedByPenNum[penNum]->addTextCharacterToOverlay(
                                      penNum, APP_CTRL_MODKEY_FROM_RAW_KEYPRESS(charData));
            }
        }

        update();
    }
}

void overlayWindow::applyReceivedMenuCommand(int penNum, quint32 menuCommand)
{
    if( APP_CTRL_IS_MENU_COMMAND(menuCommand) )
    {
        QColor newColour;
        qint32 commandWithoutTag = APP_CTRL_MENU_CMD_FROM_RX(menuCommand);
        qint32 commandClass      = MENU_ENTRY_TYPE_CLASS(menuCommand);

        qDebug() << "Command" << QString("0x%1").arg(menuCommand,0,16)
                 << "class:"  << QString("0x%1").arg(commandClass,0,16);

        if( commandClass == PEN_COLOUR_PRESET )
        {
            newColour = QColor( (menuCommand & 0xFF000000)>>24,
                                (menuCommand & 0xFF00)>>8, (menuCommand & 0xFF) );
            penColourChanged(penNum, newColour);
        }
        else if( commandClass == BACKGROUND_COLOUR_PRESET )
        {
            newColour = QColor( (menuCommand & 0xFF000000)>>24,
                                (menuCommand & 0xFF00)>>8, (menuCommand & 0xFF) );
            paperColourChanged(penNum, newColour);
        }
        else if( commandClass == BACKGROUND_TRANSPARENT )
        {
            paperColourTransparent(penNum);
        }
        else
        {
            int param = commandWithoutTag & 0xFFFF;

            switch(COMMAND_FROM_TOP_BYTE(commandWithoutTag))
            {
            case THICKNESS_PRESET: penThicknessChanged(penNum, param);     break;
            case PEN_TYPE:         setPenType(penNum, (penAction_t)param); break;
            case PEN_SHAPE:        setPenShape(penNum,(shape_t)param);     break;
            case EDIT_ACTION:      switch((pen_edit_t)param)
                                   {
                                   case EDIT_UNDO:     undoPenAction(penNum); break;
                                   case EDIT_REDO:     redoPenAction(penNum); break;
                                   case EDIT_DO_AGAIN: brushFromLast(penNum); break;
                                   default:            break;
                                   }
                                   break;

            case PAGE_ACTION:      switch((pageAction_t)param)
                                   {
                                   case PAGE_NEW:      newPage();          break;
                                   case PAGE_NEXT:     nextPage();         break;
                                   case PAGE_PREV:     prevPage();         break;
                                   case PAGE_SELECTOR: showPageSelector(); break;
                                   case PAGE_CLEAR:    clearOverlay();     break;
                                   }
                                   break;

            case STORAGE_ACTION:   switch((storageAction_t)param)
                                   {
                                   case RECORD_SESSION:            recordDiscussion();   break;
                                   case PLAYBACK_RECORDED_SESSION: playbackDiscussion(); break;
                                   case STORAGE_SNAPSHOT:          screenSnap();         break;
                                   case SNAPSHOT_GALLERY:          // Not yet implemented
                                   case STORAGE_SAVE:              break;
                                   }
                                   break;

            case STICKY_ACTION:    switch((stickyAction_t)param)
                                   {
                                   case STICKY_NEW:     addNewStickyStart(penNum);     break;
                                   case STICKY_DELETE:  deleteStickyMode(penNum);      break;
                                   case STICKY_MOVE:    startmoveofStickyNote(penNum); break;
                                   case STICKY_SNAP:    // Not yet implemented
                                   case STICKY_ARRANGE: break;
                                   }
                                   break;

            case HOST_OPTIONS:     switch((hostOptions_t)param)
                                   {
                                   case HOSTED_MODE_TOGGLE:      hostedModeToggle(penNum);
                                   case REMOTE_SESSION:          remoteNetSelectAndConnect(penNum); break;
                                   case ALLOW_REMOTE:            allowRemoteNetSession(penNum);     break;
                                   case MOUSE_AS_LOCAL_PEN:      toggleMouseAsPen();                break;
                                   case SESSION_TYPE_PRIVATE:    sessionTypePrivate(penNum);        break;
                                   case SESSION_TYPE_OVERLAY:    sessionTypeOverlay(penNum);        break;
                                   case SESSION_TYPE_WHITEBOARD: sessionTypeWhiteboard(penNum);     break;
                                   case SESSION_TYPE_STICKIES:   sessionTypeStickyNotes(penNum);    break;
                                   case SWITCH_SCREEN:           // Not yet implemented
                                   case CONTROL_PEN_ACCESS:      break;
                                   }
                                   break;

            case QUIT_APPLICATION: mainMenuQuit();               break;

            default:
                qDebug() << "Unrecognised menu command.";
            }
        }
    }
}



void overlayWindow::penDisconnectedOnHost()
{
    if( connectedToViewHost && ! viewOnlyMode )
    {
        qDebug() << "Attempt to reconnect to drive pen on host...";

        if( remoteSelector->connectPenToHost() )
        {
            netClient->updateRemotePenNum(remoteSelector->penNumberOnHost());
        }
    }
}

void overlayWindow::remoteViewCanBeShown()
{
    if( connectedToViewHost )
    {
        showIntroBackground = false;
        repaint();
    }
}



void overlayWindow::toggleMouseAsPen()
{
    qDebug() << "toggleMouseAsPen()";

    if( useMouseAsInputToo )
    {
        useMouseAsInputToo = false;
        setMouseTracking(false);

        localMouseAction->setChecked(false);
    }
    else
    {
        useMouseAsInputToo = true;

        setMouseTracking(true);
        setCursor(Qt::BlankCursor);

        // We cannot have a transparent background in mouse mode
        if( annotations->backgroundColour.alpha() < 255 )
        {
            paperColourChanged(LOCAL_MOUSE_PEN,defaultOpaqueBackgroundColour);
            replayBackgroundImage->fill(defaultOpaqueBackgroundColour);
        }

        localMouseAction->setChecked(true);
    }
}

void overlayWindow::disableWhiteboard()
{
    annotations->setBackground(Qt::transparent);
    annotations->setDefaults();
    update();
    useMouseAsInputToo = false;
    setMouseTracking(false);
    localMouseAction->setChecked(false);
}


void overlayWindow::hostedModeToggle(int requestingPen)
{
    if( inHostedMode )
    {
        // Switch the pen role for the new session type
        for(int penNum=0; penNum<MAX_SCREEN_PENS; penNum++)
        {
            penMenu[penNum]->makePeer();

            annotations->updateClientsOnPenRole(penNum, SESSION_PEER);
        }

        // and set the flag appropriately
        inHostedMode = false;
    }
    else
    {
        // Switch the pen role for the new session type
        for(int penNum=0; penNum<MAX_SCREEN_PENS; penNum++)
        {
            if( penNum != requestingPen )
            {
                penMenu[penNum]->makeFollower();

                annotations->updateClientsOnPenRole(penNum, SESSION_FOLLOWER);
            }
        }
        penMenu[requestingPen]->makeSessionLeader();

        annotations->updateClientsOnPenRole(requestingPen, SESSION_LEADER);

        inHostedMode = true;
    }

    updateClientsOnSessionState();
}


void overlayWindow::sessionTypePrivate(int requestingPen)
{
    if( remoteConnection )
    {
        qDebug() << "SEND switch session to PRIVATE.";

        menuCommand = COMMAND_TO_TOP_BYTE(HOST_OPTIONS) | SESSION_TYPE_PRIVATE;
        menuSequenceNumber ++;

        sessionType = PAGE_TYPE_PRIVATE;

        return;
    }

    if( sessionType == PAGE_TYPE_PRIVATE ) return;

    sessionType = PAGE_TYPE_PRIVATE;

    qDebug() << "Switch session to PRIVATE.";

    switch( sessionType )
    {
    case PAGE_TYPE_PRIVATE:

        qDebug() << "Session is already PRIVATE. Ignore.";
        return;

    case PAGE_TYPE_OVERLAY:

        annotations->setDefaults();
        break;

    case PAGE_TYPE_STICKIES:

        leaveStickyMode();
        break;

    case PAGE_TYPE_WHITEBOARD:

        annotations->setDefaults();
        break;
    }

    sessionType                    = PAGE_TYPE_PRIVATE;
#ifndef TABLET_MODE
    screenGrab->backgroundIsOpaque = false;
#endif
    annotations->setPageInitialBackground(Qt::transparent);
    annotations->setBackground(Qt::transparent);
    replay->setBackground(Qt::transparent);
#ifdef USE_BUTTON_STRIP
    // TODO: fix menu buttons so we can use them on the host too
    menuButtons->setSessionType(sessionType);
#endif
#ifdef USE_THREE_BUTTON_UI
    threeButtonInterface->setSessionType(sessionType);
#endif

    update();

    updateClientsOnSessionState();
}

void overlayWindow::sessionTypeOverlay(int requestingPen)
{
    if( remoteConnection )
    {
        qDebug() << "SEND switch session to OVERLAY.";

        menuCommand = COMMAND_TO_TOP_BYTE(HOST_OPTIONS) | SESSION_TYPE_OVERLAY;
        menuSequenceNumber ++;

        sessionType = PAGE_TYPE_OVERLAY;

        return;
    }

    if( sessionType == PAGE_TYPE_OVERLAY ) return;
//    if( useMouseAsInputToo )               return;

    sessionType = PAGE_TYPE_OVERLAY;

    qDebug() << "Switch session to OVERLAY.";

    switch( sessionType )
    {
    case PAGE_TYPE_PRIVATE:

        annotations->setDefaults();
        break;

    case PAGE_TYPE_OVERLAY:

        // No change, so exit.
        qDebug() << "Session already is OVERLAY. Ignore.";
        return;

    case PAGE_TYPE_STICKIES:

        leaveStickyMode();
        break;

    case PAGE_TYPE_WHITEBOARD:

        annotations->setDefaults();
        break;
    }

    sessionType                    = PAGE_TYPE_OVERLAY;
#ifndef TABLET_MODE
    screenGrab->backgroundIsOpaque = false;
#endif
    annotations->setPageInitialBackground(Qt::transparent);
    annotations->setBackground(Qt::transparent);
    replay->setBackground(Qt::transparent);
#ifdef USE_BUTTON_STRIP
    // TODO: fix menu buttons so we can use them on the host too
    menuButtons->setSessionType(sessionType);
#endif
#ifdef USE_THREE_BUTTON_UI
    threeButtonInterface->setSessionType(sessionType);
#endif

    update();

    updateClientsOnSessionState();
}



void overlayWindow::recordRemoteSessionType(int receivedType )
{
    if( ! remoteConnection ) return;

    switch( receivedType )
    {
    case PAGE_TYPE_PRIVATE:
        sessionType = PAGE_TYPE_PRIVATE;
        qDebug() << "set session to PRIVATE";
        break;

    case PAGE_TYPE_OVERLAY:
        sessionType = PAGE_TYPE_OVERLAY;
        qDebug() << "set session to OVERLAY";
        break;

    case PAGE_TYPE_STICKIES:
        sessionType = PAGE_TYPE_STICKIES;
        qDebug() << "set session to STICKIES";
        break;

    case PAGE_TYPE_WHITEBOARD:
        sessionType = PAGE_TYPE_WHITEBOARD;
        qDebug() << "set session to WHITEBOARD";
        break;
    }

#ifdef USE_BUTTON_STRIP
    // TODO: fix menu buttons so we can use them on the host too
    menuButtons->setSessionType(sessionType);
#endif

#ifdef USE_THREE_BUTTON_UI
    // TODO: fix menu buttons so we can use them on the host too
    threeButtonInterface->setSessionType(sessionType);
#endif
}


void overlayWindow::recordReceivedPenRole(int receivedRole, int penNum )
{
    // For now, this is only used in the menus, so tell the menus about it...
    if( remoteConnection && penNum == penNumOnRemoteSystem )
    {
        switch(receivedRole)
        {
        case SESSION_PEER:
#ifdef USE_BUTTON_STRIP
            menuButtons->makePeer();
#endif
#ifdef USE_THREE_BUTTON_UI
            threeButtonInterface->makePeer();
#endif
            penMenu[penNum]->makePeer();
            break;

        case SESSION_LEADER:
#ifdef USE_BUTTON_STRIP
            menuButtons->makeSessionLeader();
#endif
#ifdef USE_THREE_BUTTON_UI
            threeButtonInterface->makeSessionLeader();
#endif
            penMenu[penNum]->makeSessionLeader();
            break;

        case SESSION_FOLLOWER:
#ifdef USE_BUTTON_STRIP
            menuButtons->makeFollower();
#endif
#ifdef USE_THREE_BUTTON_UI
            threeButtonInterface->makeFollower();
#endif
            penMenu[penNum]->makeFollower();
            break;
        }
    }
}

void overlayWindow::updateClientsOnSessionState(void)
{
    annotations->updateClientsOnSessionState(sessionType);
}


void overlayWindow::sessionTypeWhiteboard(int requestingPen)
{
    if( sessionType == PAGE_TYPE_WHITEBOARD ) return;

    sessionType = PAGE_TYPE_WHITEBOARD;

    if( remoteConnection )
    {
        menuCommand = COMMAND_TO_TOP_BYTE(HOST_OPTIONS) | SESSION_TYPE_WHITEBOARD;
        menuSequenceNumber ++;

        return;
    }

    qDebug() << "Switch session to WHITEBOARD.";

    switch( sessionType )
    {
    case PAGE_TYPE_OVERLAY:

        annotations->setDefaults();
        break;

    case PAGE_TYPE_STICKIES:

        leaveStickyMode();
        break;

    case PAGE_TYPE_WHITEBOARD:

        // No change, so exit.
        qDebug() << "Session already is WHITEBOARD. Ignore.";
        return;
    }

    sessionType                    = PAGE_TYPE_WHITEBOARD;
#ifndef TABLET_MODE
    screenGrab->backgroundIsOpaque = true;
#endif
    annotations->setPageInitialBackground(defaultOpaqueBackgroundColour);
    paperColourChanged(requestingPen,defaultOpaqueBackgroundColour);
    replayBackgroundImage->fill(defaultOpaqueBackgroundColour);
#ifdef USE_BUTTON_STRIP
    menuButtons->setSessionType(sessionType);
#endif
#ifdef USE_THREE_BUTTON_UI
    threeButtonInterface->setSessionType(sessionType);
#endif

//    clearOverlay();

    update();

    updateClientsOnSessionState();
}


void overlayWindow::deleteStickyMode(int penNum)
{
    if( remoteConnection )
    {
        menuCommand = COMMAND_TO_TOP_BYTE(STICKY_ACTION) | STICKY_DELETE;
        menuSequenceNumber ++;

        return;
    }

    cursor[penNum]->setDeleteBrush();

    deleteStickyOnClick[penNum] = true;
}




void overlayWindow::startmoveofStickyNote(int penNum)
{
    if( remoteConnection )
    {
        menuCommand = COMMAND_TO_TOP_BYTE(STICKY_ACTION) | STICKY_MOVE;
        menuSequenceNumber ++;

        return;
    }

    cursor[penNum]->setMoveBrush();

    startMoveOnStickyClick[penNum] = true;
}


void overlayWindow::sessionTypeStickyNotes(int requestingPen)
{
    if( sessionType == PAGE_TYPE_STICKIES ) return;

    if( remoteConnection )
    {
        menuCommand = COMMAND_TO_TOP_BYTE(HOST_OPTIONS) | SESSION_TYPE_STICKIES;
        menuSequenceNumber ++;

        sessionType = PAGE_TYPE_STICKIES;

        return;
    }

    switch( sessionType )
    {
    case PAGE_TYPE_OVERLAY:

        qDebug() << "Switch session to STICKY from OVERLAY.";

        annotations->setDefaults();
        break;

    case PAGE_TYPE_STICKIES:

        // No change, so exit.
        qDebug() << "Session already is STICKY. Ignore.";
        return;

    case PAGE_TYPE_WHITEBOARD:

        qDebug() << "Switch session to STICKY from WHITEBOARD.";

        annotations->setDefaults();
    }

    sessionType                    = PAGE_TYPE_STICKIES;
#ifndef TABLET_MODE
    screenGrab->backgroundIsOpaque = false;
#endif
    clearOverlay();

    if( ! useMouseAsInputToo )
    {
        annotations->setPageInitialBackground(Qt::transparent);
        annotations->setBackground(Qt::transparent);
        replay->setBackground(Qt::transparent);
    }
#ifdef USE_BUTTON_STRIP
    menuButtons->setSessionType(sessionType);
#endif
#ifdef USE_THREE_BUTTON_UI
    threeButtonInterface->setSessionType(sessionType);
#endif
    update();

    updateClientsOnSessionState();

    qDebug() << "Session now STICKY";
}

int overlayWindow::currentSessionType(void)
{
    return sessionType;
}




void overlayWindow::setPenShape(int penNum, shape_t shape)
{
    if( remoteConnection )
    {
        menuCommand = COMMAND_TO_TOP_BYTE(PEN_SHAPE) | (shape & 0xFFFF);
        menuSequenceNumber ++;

        return;
    }

    qDebug() << "setPenShape()";

    switch( shape )
    {
    case SHAPE_NONE:
        cursor[penNum]->setdefaultCursor();

        penInTextMode[penNum] = false;
        brushAction[penNum]   = false;
        break;

    case SHAPE_TRIANGLE:
        annotations->explicitBrush(penNum, SHAPE_TRIANGLE, 100,0);
        replay->explicitBrush(penNum, SHAPE_TRIANGLE, 100,0);

        cursor[penNum]->setCursorFromBrush( annotations->getBrushPoints(penNum) );

        penInTextMode[penNum] = false;
        brushAction[penNum]   = true;
        break;

    case SHAPE_BOX:
        annotations->explicitBrush(penNum, SHAPE_BOX, 100,100);
        replay->explicitBrush(penNum, SHAPE_BOX, 100,100);

        cursor[penNum]->setCursorFromBrush( annotations->getBrushPoints(penNum) );

        penInTextMode[penNum] = false;
        brushAction[penNum]   = true;
        break;

    case SHAPE_CIRCLE:
        annotations->explicitBrush(penNum, SHAPE_CIRCLE, 100,0);
        replay->explicitBrush(penNum, SHAPE_CIRCLE, 100,0);

        cursor[penNum]->setCursorFromBrush( annotations->getBrushPoints(penNum) );

        penInTextMode[penNum] = false;
        brushAction[penNum]   = true;
        break;

    default:
        qDebug() << "Bad pen shape:" << shape;
        return;
    }

}


void overlayWindow::setPenType(int penNum, penAction_t type)
{
    if( remoteConnection )
    {
        menuCommand = COMMAND_TO_TOP_BYTE(PEN_TYPE) | (type & 0xFFFF);
        menuSequenceNumber ++;

        return;
    }

    penInTextMode[penNum]            = false;
    penIsDrawing[penNum]             = false;
    waitForPenButtonsRelease[penNum] = true;

    switch( type )
    {
    case DRAW_SOLID:
        if( sessionType == PAGE_TYPE_STICKIES )
        {
            if( stickyBeingUpdatedByPenNum[penNum] != NULL )
            {
                stickyBeingUpdatedByPenNum[penNum]->penTypeDraw(penNum);
            }
        }
        else
        {
            annotations->penTypeDraw(penNum);
            replay->penTypeDraw(penNum);
        }
        cursor[penNum]->setdefaultCursor();
        break;

    case DRAW_TEXT:
        if( sessionType == PAGE_TYPE_STICKIES )
        {
            if( stickyBeingUpdatedByPenNum[penNum] != NULL )
            {
                stickyBeingUpdatedByPenNum[penNum]->penTypeText(penNum);
            }
        }
        else
        {
            annotations->penTypeText(penNum);
            replay->penTypeText(penNum);
        }
        penInTextMode[penNum] = true;
        cursor[penNum]->setTextBrush();
        break;

    case ERASER:
        if( sessionType == PAGE_TYPE_STICKIES )
        {
            if( stickyBeingUpdatedByPenNum[penNum] != NULL )
            {
                stickyBeingUpdatedByPenNum[penNum]->penTypeEraser(penNum);
            }
        }
        else
        {
            annotations->penTypeEraser(penNum);
            replay->penTypeEraser(penNum);
        }
        cursor[penNum]->setdefaultCursor();
        break;

    case HIGHLIGHTER:
        if( sessionType == PAGE_TYPE_STICKIES )
        {
            if( stickyBeingUpdatedByPenNum[penNum] != NULL )
            {
                stickyBeingUpdatedByPenNum[penNum]->penTypeHighlighter(penNum);
            }
        }
        else
        {
            annotations->penTypeHighlighter(penNum);
            replay->penTypeHighlighter(penNum);
        }
        cursor[penNum]->setdefaultCursor();
        break;

    default:
        qDebug() << "Bad pen type parameter to overlayWindow::setPenType()";
    }
}


// Kick off the server thread (in replay manager). This will broadcast
// the view to any allowed connecting pens
void overlayWindow::allowRemoteNetSession(int penNum)
{
    // TODO: enable the network sender
    qDebug() << "allowRemoteNetSession: Currently all pens are network pens as no physical devices available. Pen" << penNum;
}


// TODO: check for memory leaks... looks dodgey at a quick glance.
// NB The penNum should allow the pen's menu to show errors.
void overlayWindow::remoteNetSelectAndConnect(int penNum)
{
    if( remoteSelector != NULL )
    {
        qDebug() << "Reusing existing dialogue.";
    }
    else
    {
        qDebug() << "Create remote connect dialogue";

        remoteSelector = new remoteConnectDialogue(&remoteJoinSkt, this);
        connect(remoteSelector, SIGNAL(failedRemoteConnection()),     this, SLOT(remoteConnectFailed()));
        connect(remoteSelector, SIGNAL(remoteHostSelected()),         this, SLOT(remoteSelected()));
        connect(remoteSelector, SIGNAL(connectedToRemoteHost()),      this, SLOT(remoteConnect()));
        connect(remoteSelector, SIGNAL(sendDebugRequested()),         this, SLOT(sendRemoteDebug()));
        connect(remoteSelector, SIGNAL(connectDialogueQuitProgram()), this, SLOT(mainMenuQuit()));
        connect(remoteSelector, SIGNAL(connectDialogueHelp()),        this, SLOT(help()));
    }

//#if defined(Q_OS_LINUX) && ! defined(Q_OS_ANDROID)
#if defined(Q_OS_LINUX) && ! defined(Q_OS_ANDROID)
    // Weird one for Linux & Qt 5.1.1
    this->hide();
#endif

    penConnectedToRemote = penNum;

    if( penNum == LOCAL_MOUSE_PEN )
    {
        remoteSelector->startRemoteConnection( true, "Set local pen name",
                                               annotations->getPenColour(penNum),
                                               height, width );
    }
    else
    {
        remoteSelector->startRemoteConnection( false, &(penSettings[penNum].users_name[0]),
                                               annotations->getPenColour(penNum),
                                               height, width );
    }

    remoteSelector->setFocus();

    qDebug() << "Shown remote connection dialogue.";
}


void overlayWindow::remoteSelected()
{
    // Remote system selected. Move into remote view mode.
    qDebug() << "remoteSelected()";

#if ! defined(TABLET_MODE)
    // Pause the screengrabber
    screenGrab->pauseGrabbing();
    broadcast->pauseSending();
#endif

//#if defined(Q_OS_LINUX) && ! defined(Q_OS_ANDROID)
#if ! defined(Q_OS_ANDROID)
    show();

#if defined(Q_OS_LINUX) && ! defined(Q_OS_ANDROID)
    // Weird one for Linux & Qt 5.1.1:
    this->show();
#endif
#endif
    // Try and connect as a viewer (if fail, just ignore and transmit only)

    netClient->startReceiveView(penConnectedToRemote, penNumOnRemoteSystem, replayBackgroundImage,
                                remoteSelector->hostViewAddress(), remoteSelector->hostViewPort());

    // Controller data
    hostAddr = remoteSelector->hostAddress();
    hostPort = remoteSelector->hostPort();

    setEnabled(true);

#ifndef TABLET_MODE
    // Set the display appropriately
    titleImage      = imageOfDrawnText(tr("RMT>"));
    showActionImage = true;
#else
    // show the button strip
#ifdef USE_BUTTON_STRIP
    menuButtons->show();
    qDebug() << "Call screenResized with geometry:" << width << height;
    menuButtons->screenResized(width,height);   // Added here for Qt 5.3.1
#endif
#ifdef USE_THREE_BUTTON_UI
    threeButtonInterface->show();
    threeButtonInterface->endBusyMode(); // We start in view mode, so buttons available...
    threeButtonInterface->displayMessage(tr("Connecting to remote view"),4000);
    qDebug() << "Call screenResized with geometry:" << width << height;
    threeButtonInterface->screenResized(width,height);   // Added here for Qt 5.3.1
#endif
    width = desktop->availableGeometry().width(); // - menuButtons->width();
    qDebug() << "Done menuButtons->show();";
#endif
    // Hide the local pens
    for(int p=0; p<MAX_SCREEN_PENS; p++)
    {
        if( penMenu[p]->isVisible() )
        {
            qDebug() << "Hide menu for pen" << p << "as now in remote session.";

            penMenu[p]->hide();
        }
        cursor[p]->hide();          // Hide all the other pens
        dialogCursor[p]->hide();    // Hide menus
        penIsDrawing[p] = false;    // Stop any ongoing draw activities
    }

    connectedToViewHost = true; // TODO: Do we check for this failing (timeout, I'd assume)...
    remoteConnection    = true;
    showActionImage     = false;
    viewOnlyMode        = true;
    privOnlyMode        = false;

    annotationsImage->fill(Qt::transparent);

    update();

    return;
}


bool overlayWindow::connectToRemoteForEdit()
{
    // Only do this if we are connected
    if( ! remoteConnection )
    {
        qDebug() << "connectToRemoteForEdit() failed as no view connection.";
        return false;
    }

    // Now, request to connect to the remote. TODO: move this to an async task
    bool connected = remoteSelector->connectPenToHost();

    if( ! connected )
    {
        qDebug() << "connectToRemoteForEdit() failed as negotiation failed.";
#ifdef USE_THREE_BUTTON_UI
        threeButtonInterface->displayMessage("Failed to connect to remote host.",5000);
        threeButtonInterface->setViewMode();
        threeButtonInterface->endBusyMode();
#endif
        viewOnlyMode = true;
        return false;
    }
    else
    {
        qDebug() << "connectToRemoteForEdit() SUCCESS! remote pen number ="
                 << remoteSelector->penNumberOnHost();

        netClient->updateRemotePenNum(remoteSelector->penNumberOnHost());
        netClient->doCheckForPenPresent();
#ifdef USE_THREE_BUTTON_UI
        threeButtonInterface->endBusyMode();
#endif

        viewOnlyMode = false;
        return true;
    }
}


bool overlayWindow::disconnectFromRemoteEdit()
{
    // Only do this if we are connected
    if( ! remoteConnection || viewOnlyMode )
    {
        return false;
    }

    // Now, request to connect to the remote. TODO: move this to an async task
    bool disconnected = remoteSelector->disconnectPenFromHost();

    if(disconnected)
    {
        viewOnlyMode = true;
        netClient->dontCheckForPenPresent();

        return true;
    }
    else
    {
        viewOnlyMode = false;
        return false;
    }
}


// Slot indicating that the dialogue has managed to connect to a remote system's
// sysTray. We will do the view thang here.
void overlayWindow::remoteConnect()
{
    qDebug() << "remoteConnect()";

#if ! defined(TABLET_MODE)
    // Pause the screengrabber
    screenGrab->pauseGrabbing();
    broadcast->pauseSending();
#endif

    // Try and prevent presses hanging around...
    mouseButtons         = 0;

    penNumOnRemoteSystem = remoteSelector->penNumberOnHost();

//#if defined(Q_OS_LINUX) && ! defined(Q_OS_ANDROID)
#if ! defined(Q_OS_ANDROID)
    show();

#if defined(Q_OS_LINUX) && ! defined(Q_OS_ANDROID)
    // Weird one for Linux & Qt 5.1.1:
    this->show();
#endif
#endif
    // Try and connect as a viewer (if fail, just ignore and transmit only)

    netClient->startReceiveView(penConnectedToRemote, penNumOnRemoteSystem, replayBackgroundImage,
                                remoteSelector->hostViewAddress(), remoteSelector->hostViewPort());

    // Controller data
    hostAddr = remoteSelector->hostAddress();
    hostPort = remoteSelector->hostPort();

    setEnabled(true);

#ifndef TABLET_MODE
    // Set the display appropriately
    titleImage      = imageOfDrawnText(tr("RMT>"));
    showActionImage = true;
#else
    // show the button strip
#ifdef USE_BUTTON_STRIP
    menuButtons->show();
    qDebug() << "Call screenResized with geometry:" << width << height;
    menuButtons->screenResized(width,height);   // Added here for Qt 5.3.1
#endif
#ifdef USE_THREE_BUTTON_UI
    threeButtonInterface->show();
    threeButtonInterface->endBusyMode();
    qDebug() << "Call screenResized with geometry:" << width << height;
    threeButtonInterface->screenResized(width,height);   // Added here for Qt 5.3.1
#endif
    width = desktop->availableGeometry().width(); // - menuButtons->width();
    qDebug() << "Done menuButtons->show();";
#endif
    // Hide the local pens
    for(int p=0; p<MAX_SCREEN_PENS; p++)
    {
        if( penMenu[p]->isVisible() )
        {
            qDebug() << "Hide menu for pen" << p << "as now in remote session.";

            penMenu[p]->hide();
        }
        cursor[p]->hide();          // Hide all the other pens
        dialogCursor[p]->hide();    // Hide menus
        penIsDrawing[p] = false;    // Stop any ongoing draw activities
    }

    connectedToViewHost = true; // TODO: Do we check for this failing (timeout, I'd assume)...
    remoteConnection    = true;
    showActionImage     = false;
    viewOnlyMode        = false;

    annotationsImage->fill(Qt::transparent);

    update();

    return;
}

void overlayWindow::remoteConnectFailed()
{
    QMessageBox::warning(this,tr("Failed"),tr("Failed to connect to the requested system."));
}


void overlayWindow::remoteResolutionChanged(int w, int h)
{
    // Pick up the new resolution from the scaled background image
    //annotationsImage->width()  annotationsImage->height()
#if 0
    float xScale = 1.0*width/w;
    float yScale = 1.0*height/h;

    nonLocalViewScale    = xScale > yScale ? xScale : yScale;
    nonLocalViewScaleMax = nonLocalViewScale;
#endif
    nonLocalViewCentreX  = w / 2;
    nonLocalViewCentreY  = h / 2;

    QImage scale;

    scale                  = annotationsImage->scaled(w,h);
    *annotationsImage      = scale;
    scale                  = highlights->scaled(w,h);
    *highlights            = scale;
    scale                  = replayBackgroundImage->scaled(w,h);
    *replayBackgroundImage = scale;

    update();
}

void overlayWindow::netClientUpdate(void)
{
    update();
}

void overlayWindow::movePenFromReceivedData(int penNum, int x, int y, bool show)
{
    if( penNum<0 || penNum>= MAX_SCREEN_PENS )
    {
        qDebug() << "movePenFromReceivedData: bad penNum:" << penNum;
        return;
    }

    if( show )
    {
        cursor[penNum]->moveCentre(x,y);
        cursor[penNum]->show();
    }
    else
    {
        cursor[penNum]->hide();
    }
}

#if 0
void overlayWindow::viewReceiveSocketConnected(void)
{
    qDebug() << "Viewer connected to host...";

    if( connectedToViewHost )
    {
        qDebug() << "### received connected() while already connected to host.";
        return;
    }

    // Tell replayManager to start doing a receive thing.
    netClient->startReceiveView(penConnectedToRemote, replayBackgroundImage);

    // Claim the other local pens are inactive, so they can be restarted by readData on stop
    for( int p=0; p<MAX_SCREEN_PENS; p++ )
    {
        cursor[p]->hide();
        penWasActive[p] = false;
    }
    // Except show the one we are sending without the lag...
    cursor[penConnectedToRemote]->show();

    connectedToViewHost = true;

    update();
}

void overlayWindow::viewReceiveSocketDisconnected(void)
{
    if( connectedToViewHost )
    {
        qDebug() << "TODO: investigate all that is required to reset to local use.";

        // Tell replayManager to stop doing a receive thing
        netClient->stopReceiveView();

        connectedToViewHost = false;
#ifdef Q_OS_ANDROID
        remoteNetSelectAndConnect(LOCAL_MOUSE_PEN);
#endif
    }
}
#endif


void overlayWindow::addNewStickyStart(int penNum)
{
    if( remoteConnection )
    {
        menuCommand = COMMAND_TO_TOP_BYTE(STICKY_ACTION) | STICKY_NEW;
        menuSequenceNumber ++;

        return;
    }

    newStickySize[penNum] = 200;

    // Set the cursor for this pen to a yellow square
    cursor[penNum]->setStickyAddBrush(newStickySize[penNum], Qt::yellow);

    // Set flag so that on left click, we know to do a sticky create
    addingNewStickyMode[penNum] = true;
}

bool overlayWindow::addNewSticky(int penNum, int stickySize, QPoint pos)
{
    if( ! addingNewStickyMode[penNum] ) return false;

    // TODO: snap mode

    QRect rectPos = QRect(pos,QSize(newStickySize[penNum],newStickySize[penNum]));

    // If overlaps a current sticky, don't do it and say so by returning false
    for(int n=0; n<notes.count(); n++)
    {
        QRect noteRect = QRect(QPoint(notes[n]->pos().x()+(notes[n]->size().width()/2),
                                      notes[n]->pos().y()+(notes[n]->size().height()/2)),
                               notes[n]->size());

        if( rectPos.intersects( noteRect ) )
        {
            qDebug() << "Prevent overlapping notes.";
            return false;
        }
    }

    // Create a new sticky window and add it to the
    QColor currentColours[MAX_SCREEN_PENS];
    int    currentWidths[MAX_SCREEN_PENS];

    for(int p=0; p<MAX_SCREEN_PENS; p++)
    {
        currentColours[p] = cursor[p]->getCurrentColour();
        currentWidths[p]  = cursor[p]->getCurrentPenSize();
    }

    stickyNote *sn = new stickyNote(newStickySize[penNum], newStickySize[penNum],
                                    penNum, penInTextMode, brushAction,
                                    currentColours, currentWidths,
                                    Qt::yellow, this);
    sn->move(cursor[penNum]->pos());
    sn->show();

    notes.append(sn);

    // Allows a broadcast to client displays
    annotations->addStickyNote(stickySize, pos, penNum);

    // Bump our pens above everything to make 'em above the new note
    for(int pen=0; pen<MAX_SCREEN_PENS; pen++)
    {
        if( cursor[pen]->isVisible() )
        {
            cursor[pen]->raise();
        }
    }

    return true;
}

// Delete stickies and do any required cleanup.
void overlayWindow::leaveStickyMode(void)
{
    for(int penNum=0; penNum<MAX_SCREEN_PENS; penNum++ )
    {
        if( addingNewStickyMode[penNum] )
        {
            addingNewStickyMode[penNum] = false;
            cursor[penNum]->setdefaultCursor();
            waitForPenButtonsRelease[penNum] = true;
        }

        stickyBeingUpdatedByPenNum[penNum] = NULL;
        penIsDrawing[penNum]               = false;
    }

    for(int i=0; i<notes.size(); i++)
    {
        notes[i]->hide();
    }
    notes.clear();
}


// Called by menuStructure, on BACKGROUND_COLOUR_*

void overlayWindow::paperColourChanged(int penNum, QColor newColour)
{
    if( remoteConnection )
    {
        menuCommand = BACKGROUND_COLOUR_PRESET     | ((newColour.red()&255) << 24) |
                      (newColour.green()&255) << 8 | (newColour.blue()&255);
        menuSequenceNumber ++;

        return;
    }

    if( useMouseAsInputToo && newColour.alpha()<255 )
    {
        qDebug() << "Cannot set a transparent background if using mouse tracking.";
        return;
    }

    if( sessionType == PAGE_TYPE_STICKIES )
    {
        if( stickyBeingUpdatedByPenNum[penNum] != NULL )
        {
            stickyBeingUpdatedByPenNum[penNum]->setNoteColour(penNum, newColour);
        }
        else qDebug() << "Sticky colour ignored as no sticky selected.";
    }
    else
    {
        annotations->setBackground(newColour);
        replay->setBackground(newColour);
    }
}

void overlayWindow::privPaperColourChanged(QColor newColour)
{
    drawTrace->fillPrivateBackground(newColour);
}

void overlayWindow::paperColourTransparent(int penNum)
{
    if( remoteConnection )
    {
        menuCommand = BACKGROUND_TRANSPARENT;
        menuSequenceNumber ++;

        qDebug() << "Send paperColourTransparent:" << QString("0x%1").arg(menuCommand,0,16);

        return;
    }

    qDebug() << "paperColourTransparent" << penNum;

    if( useMouseAsInputToo )
    {
        qDebug() << "Cannot set a transparent background if using mouse tracking.";
        return;
    }

    if( sessionType == PAGE_TYPE_STICKIES )
    {
        if( stickyBeingUpdatedByPenNum[penNum] != NULL )
        {
            stickyBeingUpdatedByPenNum[penNum]->setNoteColour(penNum, Qt::transparent);
        }
        else qDebug() << "Sticky colour ignored as no sticky selected.";
    }
    else
    {
        annotations->setBackground(Qt::transparent);
        replay->setBackground(Qt::transparent);
    }
}

void overlayWindow::penThicknessChanged(int penNum, int newThickness)
{
    if( remoteConnection )
    {
        menuCommand = COMMAND_TO_TOP_BYTE(THICKNESS_PRESET) | (newThickness&0xFFFF);
        menuSequenceNumber ++;

        drawTrace->setPenThickness(newThickness);
        drawTrace->setPPenThickness(newThickness);    //pn test

        return;
    }

    cursor[penNum]->updatePenSize(newThickness);
    annotations->setPenThickness(penNum,newThickness);
    replay->setPenThickness(penNum,newThickness);
}

void overlayWindow::undoPenAction(int penNum)
{
    if( remoteConnection )
    {
        menuCommand = COMMAND_TO_TOP_BYTE(EDIT_ACTION) | EDIT_UNDO;
        menuSequenceNumber ++;

        return;
    }

    annotations->undoPenAction(penNum);
    replay->undoPenAction(penNum);
}

void overlayWindow::undoPrivateLines(void)
{
    drawTrace->privateNoteUndo();
}

void overlayWindow::redoPenAction(int penNum)
{
    if( remoteConnection )
    {
        menuCommand = COMMAND_TO_TOP_BYTE(EDIT_ACTION) | EDIT_REDO;
        menuSequenceNumber ++;

        return;
    }

    annotations->redoPenAction(penNum);
    replay->redoPenAction(penNum);
}

bool overlayWindow::brushFromLast(int penNum)
{
    if( remoteConnection )
    {
        menuCommand = COMMAND_TO_TOP_BYTE(EDIT_ACTION) | EDIT_DO_AGAIN;
        menuSequenceNumber ++;

        return true;
    }

    if( annotations->brushFromLast(penNum) )
    {
        replay->brushFromLast(penNum);
        brushAction[penNum] = true;

        // I know, nasty to explicitly reference cursor from here.
        cursor[penNum]->setCursorFromBrush(annotations->getBrushPoints(penNum));

        return true;
   }
   else
   {
        qDebug() << "Failed to get brush from last!";
        return false;
   }
}




// Called by menuStructure, on BACKGROUND_COLOUR_*

void overlayWindow::penColourChanged(int penNum, QColor newColour)
{
    if( remoteConnection )
    {
        qDebug() << "Send new colour to host...";

        menuCommand = PEN_COLOUR_PRESET            | ((newColour.red()&255) << 24) |
                      (newColour.green()&255) << 8 | (newColour.blue()&255);
        menuSequenceNumber ++;

        drawTrace->setPenColour(newColour);
        drawTrace->setPPenColour(newColour);

        localPenColour = newColour;

        return;
    }

    if( sessionType == PAGE_TYPE_STICKIES )
    {
        for(int sn=0; sn<notes.size(); sn ++)
        {
            cursor[penNum]->updatePenColour(newColour);
            notes[sn]->setPenColour(penNum, newColour);
        }
    }
    else
    {
        cursor[penNum]->updatePenColour(newColour);
        annotations->setPenColour(penNum,newColour);
        replay->setPenColour(penNum,newColour);
    }
}

QColor overlayWindow::getLocalPenColour(void)
{
    return localPenColour;
}


void overlayWindow::mainMenuDisconnect(void)
{
    if( remoteConnection )
    {
        remoteConnection    = false;
        showActionImage     = false;
        connectedToViewHost = false;

        netClient->stopReceiveView();
#ifndef TABLET_MODE
        if( ! sysTrayDetected )
        {
            screenGrab->resumeGrabbing();
            broadcast->resumeSending();
        }
#endif
#ifdef TABLET_MODE
        // Kick off the connect to remote host dialogue as this is always a remote session
        enableTextInputWindow(false);
#ifdef USE_BUTTON_STRIP
        menuButtons->hide();
#endif
#ifdef USE_THREE_BUTTON_UI
        threeButtonInterface->hide();
#endif

        width  = desktop->availableGeometry().width();
        height = desktop->availableGeometry().height();
        qDebug() << "adjust remote connet dialogue position";
        remoteSelector->setStatusString(tr("Disconnected. Select new remote session to join."));
        remoteSelector->reStartRemoteConnection(height,width);

        // TODO: remove this, but there some bugs that are stopping this work correctly.
        mainMenuQuit();
#endif

        // Restore the last page view before the connection.
        qDebug() << "Fill in default background";
        annotationsImage->fill(defaultOpaqueBackgroundColour);
        highlights->fill(defaultOpaqueBackgroundColour);

        nonLocalViewScale    = 1.0;
        nonLocalViewScaleMax = 1.0;
        nonLocalViewCentreX  = width/2;
        nonLocalViewCentreY  = height/2;

        qDebug() << "and update";
        update();
    }
}


void overlayWindow::mainMenuQuit(void)
{
    qDebug() << "mainMenuQuit";

    if( replay->currentlyRecording() )
        replay->finishUpSaving();

    annotationsImage->fill(defaultOpaqueBackgroundColour);
    repaint();

    if( fileDialogue->isVisible() )
        fileDialogue->close();

    for(int p=0; p<MAX_SCREEN_PENS; p++)
    {
        if( cursor[p]->isVisible() )
            cursor[p]->close();
    }

    qDebug() << "Quit remote selector.";

    if( remoteSelector != NULL )
    {
        remoteSelector->hide();
        remoteSelector->close();
    }

    qDebug() << "Close main window.";

    close();
}

void overlayWindow::sendRemoteDebug()
{
    qDebug() << "overlayWindow::sendRemoteDebug()";

    QNetworkAccessManager *m = sendDebugToServer();

    connect(m, SIGNAL(finished(QNetworkReply *)), this, SLOT(sendRemoteDebugDone(QNetworkReply *)) );
}

void overlayWindow::sendRemoteDebugDone(QNetworkReply *reply)
{
    sendDebugToServerComplete();

    QMessageBox::information(this, tr("Send complete"), tr("Debug data from previous run successfully sent to server"));
}


void overlayWindow::setUpDialogueCursors(QWidget *parent)
{
    QRect bounds;

    // And let's copy the pen states
    for(int p=0; p<MAX_SCREEN_PENS; p++)
    {
        // NB Only the colour (we don't need to know how big the brush would look)
        dialogCursor[p]->updatePenColour(annotations->getPenColour(p));

        // Show any pens that are currently over the dialogue
        dialogCursor[p]->hide();

        if( cursor[p]->isVisible() )
        {
            bounds = parent->geometry();

            if( bounds.contains(cursor[p]->x(),cursor[p]->y()) )
            {
                dialogCursor[p]->moveCentre(cursor[p]->x() - parent->x(),
                                            cursor[p]->y() - parent->y() );
                dialogCursor[p]->show();
            }
        }
    }
}


void overlayWindow::localScreenSnap(void)
{
    if( replayBackgroundImage != NULL )
    {
        QPixmap *p = new QPixmap(replayBackgroundImage->width(),replayBackgroundImage->height());
        if( p != NULL )
        {
            QStringList locs  = QStandardPaths::standardLocations(QStandardPaths::PicturesLocation);

            if( locs.count() > 0 )
            {
                QPainter paint;
                paint.begin( p );
                qDebug() << "Test 0";
#if defined(TABLET_MODE)
                paint.drawImage(QPoint(0,0),*replayBackgroundImage);
#endif
#ifndef TABLET_MODE
// This should crash on tablets as screenGrab isn't there
                qDebug() << "Test 1";
                screenSnapData = screenGrab->currentBackground();
                QImage autoScreenSnap = screenSnapData.toImage();
                paint.drawImage(QPoint(0,0),autoScreenSnap);
#endif
                paint.end();
                const QDateTime now = QDateTime::currentDateTime();
                const QString dateTimestamp = now.toString(QLatin1String("yyyy-MM-dd"));
                const QString timeTimestamp = now.toString(QLatin1String("hh-mm-ss"));
                const QString filename = QString::fromLatin1("freestyle-%1").arg(timeTimestamp);
//                QDir cosnetaDir();
                QDir todayDir(locs[0] + QDir::separator() + "Cosneta " + dateTimestamp);
//                QDir cosnetaDir(locs[0] + QDir::separator() + "Cosneta");
//                QDir todayDir(cosnetaDir.path() + QDir::separator() + dateTimestamp);
//                if(!cosnetaDir.exists()) {
//                    cosnetaDir.mkpath(".");
//                } else
//                    qDebug() << "Cosneta folder already exists!";

                if(!todayDir.exists())
                {
                    todayDir.mkpath(".");
                }
                else
                    qDebug() << "Today's folder already exists!";
                QString     fname = todayDir.path() + QDir::separator() + filename + ".png";
                qDebug() << fname;
                p->save(fname,"png");
                free( p );

                notifyAndroidMediaScanner(fname);

#ifdef USE_THREE_BUTTON_UI
                threeButtonInterface->displayMessage(tr("Screenshot snapped"),4000);
#endif
            }
            else
            {
                qDebug() << "No standard location for pictures found!";
            }
        }
        else
        {
            qDebug() << "Failed to allocate QPixmap";
        }
    }
    else qDebug() << "NO IMAGE!!!";
}

#ifdef Q_OS_ANDROID
#include <QtAndroidExtras/QAndroidJniObject>
#endif

void overlayWindow::notifyAndroidMediaScanner(QString filename)
{
    qDebug() << "notifyAndroidMediaScanner(" << filename << ")";
#ifdef Q_OS_ANDROID
    QAndroidJniObject javaFileUriName = QAndroidJniObject::fromString(filename);

    qDebug() << "notifyAndroidMediaScanner() call static method: informOfNewFile()";

    QAndroidJniObject::callStaticMethod<void>("com/cosneta/freeStyleQt/FreeStyleQt",
                                              "informOfNewFile",
                                              "(Ljava/lang/String;)V",
                                              javaFileUriName.object<jstring>());

    qDebug() << "notifyAndroidMediaScanner() finished call of static method: informOfNewFile()";
#endif
}


void overlayWindow::screenSnap(void)
{
    if( remoteConnection )
    {
        menuCommand = COMMAND_TO_TOP_BYTE(STORAGE_ACTION) | STORAGE_SNAPSHOT;
        menuSequenceNumber ++;

        return;
    }

#ifdef TABLET_MODE
    qDebug() << "Local screen snap not allowed. Error";
    return;
#else
    // Hide pens and menus

    bool penVisible[MAX_SCREEN_PENS];
    bool menuVisible[MAX_SCREEN_PENS];
    int  penNum;

    for(penNum=0; penNum<MAX_SCREEN_PENS; penNum++)
    {
        penVisible[penNum] = false;
        if( cursor[penNum]->isVisible() )
        {
            cursor[penNum]->hide();
            penVisible[penNum] = true;
        }

        menuVisible[penNum] = false;
        if( penMenu[penNum]->isVisible() )
        {
            qDebug() << "Hide menu for pen" << penNum << "as taking a screenSnap.";

            penMenu[penNum]->hide();
            menuVisible[penNum] = true;
        }
    }

    // Process Qt events to allow the menu to hidden before taking the snapshot.
    QApplication::processEvents();

    screenSnapData = screenGrab->currentBackground();

    // Restore pens and menus

    for(penNum=0; penNum<MAX_SCREEN_PENS; penNum++)
    {
        if( penVisible[penNum] )
        {
            cursor[penNum]->show();
        }

        if( menuVisible[penNum] )
        {
            penMenu[penNum]->show();
            penMenu[penNum]->raise();
        }
    }

    update();

    fileDialogue->setWindowTitle(tr("Save Screen Snapshot"));
    fileDialogue->setNameFilter(tr("PNG Image files (*.png)"));
    fileDialogue->setFileMode(QFileDialog::AnyFile); // ExistingFile for read
    fileDialogue->setWindowModality(Qt::NonModal);
    fileDialogue->selectFile(fileDialogue->directory().canonicalPath()+"/snapshot01.png");
    fileDialogue->setAcceptMode(QFileDialog::AcceptSave);

    fileDialogueAction = FD_SCREENSNAP;

#ifdef Q_OS_LINUX
    // Weird one for Linux & Qt 5.1.1:
    this->hide();
#endif

    fileDialogue->show();
    fileDialogue->activateWindow();

    setUpDialogueCursors(fileDialogue);
#endif
}


void overlayWindow::fileDialogReject()
{
    qDebug() << "overlayWindow::fileDialogReject()";
    switch( fileDialogueAction )
    {
    case FD_SCREENSNAP:

        screenSnapData = QPixmap(); // Frees the memory
        break;

    case FD_RECORD_SEL:
    case FD_REPLAY_SEL:
    case FD_NO_ACTION:
        break;
    }

    fileDialogueAction = FD_NO_ACTION;

#ifdef Q_OS_LINUX
    // Weird one for Linux & Qt 5.1.1:
    this->show();
#endif
}



void overlayWindow::fileDialogAccept()
{
    qDebug() << "overlayWindow::fileDialogAccept()";

    QString fileName;
    bool    error    = false;

    if( fileDialogue->selectedFiles().count() != 1 )
    {
        error = true;
    }
    else
    {
        fileName = fileDialogue->selectedFiles().first();
    }

    if( error || fileName.isEmpty() )
    {
        fileDialogReject();
        return;
    }

    switch( fileDialogueAction )
    {
    case FD_SCREENSNAP:

        fileDialogueAction = FD_NO_ACTION;

        if( ! screenSnapData.save(fileName, "png") )
        {
            qDebug() << "Failed to save snapshot!";
        }
        screenSnapData = QPixmap(); // Free the memory

        break;

    case FD_RECORD_SEL:

        fileDialogueAction = FD_NO_ACTION;

        // Hide our window so we have a clean background (otherwise may get pens on background)
        this->hide();

        // Wait for 0.5 sec to allow file dialogue to fade (it sometime it taks a while)
        waitMS(500);

        // Clean up on failure
        if( ! replay->saveDiscussion(fileName) )
        {
            this->show();

            // TODO: use a pen accessible function
            QMessageBox::warning(this,tr("Error"),tr("Failed to start saving the discussion."));

            break;
        }

        // Clear current overlay
        annotations->setDefaults();
#if 0
        for(int p=0; p<MAX_SCREEN_PENS; p++)
        {
            annotations->setPenColour(p, penMenu[p]->getColour());
        }
#endif
        titleImage      = imageOfDrawnText(tr("REC>"));
        showActionImage = true;

        // Background should have been snapped, so we can display the overlay again.
        this->show();

        qDebug()  << "overlay sz [" << annotationsImage->width() << "," << annotationsImage->height()
                  << "] background sz [" << replayBackgroundImage->width() << "," <<
                     replayBackgroundImage->height() << "]";

        update();

        break;

    case FD_REPLAY_SEL:

        fileDialogueAction = FD_NO_ACTION;

        if( ! replay->replayDiscussion(fileName,replayBackgroundImage) )
        {
            QMessageBox::warning(this,tr("Error"),tr("Failed to replay selected discussion."));

            break;
        }

        // Ensure that we're not paused. and enable playback
        playingBackPaused      = false;
        playingRecordedSession = true;

        // Set the jog-shuttle up:
        jogShuttleImage = imageOfJogShuttle(width,true);
        jogShuttlePos   = QRect( (width-jogShuttleImage->width())/2,height*9/10,
                                 jogShuttleImage->width(),jogShuttleImage->height() );

        // Hide the current menu stuff: only controls are now jog shuttle
        for(int p=0; p<MAX_SCREEN_PENS; p++)
        {
            if( penMenu[p]->isVisible() )
            {
                qDebug() << "Hide menu for pen" << p << "as starting replay";

                penMenu[p]->hide();
            }
        }

        titleImage      = imageOfDrawnText(tr("PLAY>"));
        showActionImage = true;

        annotations->setDefaults();

        // TODO: create a timer so we can try to match real time
        playbackRefire->singleShot(1,this,SLOT(playbackNextStep()));

        break;

    case FD_NO_ACTION:

        // Err, let's do nothing. :P
        break;
    }

    this->activateWindow();

#ifdef Q_OS_LINUX
    // Weird one for Linux & Qt 5.1.1:
    this->show();
#endif
}


void overlayWindow::recordDiscussion(void)
{
    if( remoteConnection )
    {
        menuCommand = COMMAND_TO_TOP_BYTE(STORAGE_ACTION) | RECORD_SESSION;
        menuSequenceNumber ++;

        return;
    }

    QString fileName;

    if( replay->currentlyRecording() )
    {
        replay->finishUpSaving();

        titleImage      = imageOfDrawnText("Idle");
        showActionImage = false;

        return;
    }

    fileDialogue->setWindowTitle(tr("Save Discussion"));
    fileDialogue->setNameFilter(tr("Cosneta Discussion files (*.cfs)"));
    fileDialogue->setFileMode(QFileDialog::AnyFile); // ExistingFile for read
    fileDialogue->setWindowModality(Qt::NonModal);
    fileDialogue->selectFile(fileDialogue->directory().canonicalPath()+"/discussion01.cfs");
    fileDialogue->setAcceptMode(QFileDialog::AcceptSave);

    fileDialogueAction = FD_RECORD_SEL;

#ifdef Q_OS_LINUX
    // Weird one for Linux & Qt 5.1.1:
    this->hide();
#endif

    fileDialogue->show();
    fileDialogue->activateWindow();

    setUpDialogueCursors(fileDialogue);
}


void overlayWindow::waitMS(int msec)
{
    QTime stopTime = QTime::currentTime().addMSecs(msec);

    while( QTime::currentTime() < stopTime )
    {
        QCoreApplication::processEvents(QEventLoop::AllEvents,10); // Wait for up to 0.01 sec
    }
}

void overlayWindow::playbackRewind(void)
{
    if( ! replay->currentlyPlaying() ) return;

    replay->rewindPlayback();

    update();
}

void overlayWindow::playbackStop(void)
{
    if( ! replay->currentlyPlaying() ) return;

    // Stop playback
    replay->stopPlayback();

    playingRecordedSession = false;

    titleImage      = imageOfDrawnText("Idle");
    showActionImage = false;

    // Set overlay(s) to transparent

    if( useMouseAsInputToo )
    {
        replayBackgroundImage->fill(defaultOpaqueBackgroundColour);
    }
    else
    {
        replayBackgroundImage->fill(Qt::transparent);
    }
    annotations->setBackground(Qt::transparent);
    highlights->fill(Qt::transparent);

    update();
}

void overlayWindow::playbackTogglePlayPause(void)
{
    if( ! replay->currentlyPlaying() ) return;

    jogShuttleImage = imageOfJogShuttle(width,playingBackPaused);
    jogShuttlePos   = QRect( (width-jogShuttleImage->width())/2,height*9/10,
                             jogShuttleImage->width(),jogShuttleImage->height() );

    // NB Change here to use old value in regenerating the image.
    playingBackPaused = ! playingBackPaused;

    update();
}


void overlayWindow::playbackDiscussion(void)
{
    if( remoteConnection )
    {
        menuCommand = COMMAND_TO_TOP_BYTE(STORAGE_ACTION) | PLAYBACK_RECORDED_SESSION;
        menuSequenceNumber ++;

        return;
    }

    QString fileName;

    if( replay->currentlyPlaying() )
    {
        playbackStop();

        return;
    }

    fileDialogue->setWindowTitle(tr("Replay Discussion"));
    fileDialogue->setNameFilter(tr("Cosneta Discussion files (*.cfs)"));
    fileDialogue->setFileMode(QFileDialog::AnyFile); // ExistingFile for read
    fileDialogue->setWindowModality(Qt::NonModal);
    fileDialogue->selectFile(fileDialogue->directory().canonicalPath()+"/discussion01.cfs");
    fileDialogue->setAcceptMode(QFileDialog::AcceptOpen);

    fileDialogueAction = FD_REPLAY_SEL;

#ifdef Q_OS_LINUX
    // Weird one for Linux & Qt 5.1.1:
    this->hide();
#endif

    fileDialogue->show();
    fileDialogue->activateWindow();

    setUpDialogueCursors(fileDialogue);
}


// TODO: add a timer to get time passed during update to reduce delay at end.
void overlayWindow::playbackNextStep(void)
{
    qDebug("playbackNextStep()");

    if( ! playingRecordedSession ) return;

    int delay;

    if( ! playingBackPaused )
    {
        // Draw the current frame
        delay = replay->nextPlaybackStep();

        if( delay<0 )
        {
            replay->stopPlayback();

            // Failed to read data, or reached the end of the session

            titleImage       = imageOfDrawnText("Idle");
             showActionImage = false;

            playingRecordedSession = false;

            // Reset the pens to the default state
            //resetPens();

            // Set overlay(s) to transparent

            if( useMouseAsInputToo )
            {
                replayBackgroundImage->fill(defaultOpaqueBackgroundColour);
            }
            else
            {
                replayBackgroundImage->fill(Qt::transparent);
            }
            annotations->setBackground(Qt::transparent);
            highlights->fill(Qt::transparent);

            return;
        }
    }
    else
    {
        delay = 50;    // Poll for start enabled at 20Hz
    }

    // Success, timeout until next update
    playbackRefire->singleShot(delay,this,SLOT(playbackNextStep()));
}



#define BUTTON_ACTION(current,prev,mask) \
  ( (((current) & (mask)) != 0) && (((prev) & (mask)) == 0 ) ? CLICKED  : \
    (((current) & (mask)) == 0) && (((prev) & (mask)) != 0 ) ? RELEASED : NO_ACTION )

void overlayWindow::activePenUpdate(pen_state_t *state,  int penNum,
                                    int          xLoc[], int yLoc[])
{
    if( waitForPenButtonsRelease[penNum] )
    {
        if( state->buttons == 0 )
        {
            waitForPenButtonsRelease[penNum] = false;
        }
        else
        {
            // Mask out button presses until they've all been released.
            state->buttons = 0;
        }
    }

    bool             selectButton   = ((state->buttons &  BUTTON_LEFT_MOUSE) != 0);
    bool             wasSelectButton = (lastButtons[penNum] & BUTTON_LEFT_MOUSE) != 0;
    buttonsActions_t buttons;

    buttons.leftMouse             = state->buttons & BUTTON_LEFT_MOUSE   ? true : false;
    buttons.centreMouse           = state->buttons & BUTTON_MIDDLE_MOUSE ? true : false;
    buttons.rightMouse            = state->buttons & BUTTON_RIGHT_MOUSE  ? true : false;
    buttons.scrollPlus            = state->buttons & BUTTON_SCROLL_DOWN  ? true : false;
    buttons.scrollMinus           = state->buttons & BUTTON_SCROLL_UP    ? true : false;
    buttons.modeToggle            = state->buttons & BUTTON_MODE_SWITCH  ? true : false;
    buttons.precisionToggle       = false;
    buttons.gestureToggle         = state->buttons & BUTTON_OPT_A       ? true : false;

    buttons.leftMouseAction       = BUTTON_ACTION(state->buttons,lastButtons[penNum],BUTTON_LEFT_MOUSE);
    buttons.centreMouseAction     = BUTTON_ACTION(state->buttons,lastButtons[penNum],BUTTON_MIDDLE_MOUSE);
    buttons.rightMouseAction      = BUTTON_ACTION(state->buttons,lastButtons[penNum],BUTTON_RIGHT_MOUSE);
    buttons.scrollPlusAction      = BUTTON_ACTION(state->buttons,lastButtons[penNum],BUTTON_SCROLL_DOWN);
    buttons.scrollMinusAction     = BUTTON_ACTION(state->buttons,lastButtons[penNum],BUTTON_SCROLL_UP);
    buttons.modeToggleAction      = BUTTON_ACTION(state->buttons,lastButtons[penNum],BUTTON_MODE_SWITCH);
    buttons.precisionToggleAction = NO_ACTION;  // BUTTON_PRECISION ?
    buttons.gestureToggleAction   = BUTTON_ACTION(state->buttons,lastButtons[penNum],BUTTON_OPT_A);

    lastButtons[penNum] = state->buttons;

    xLoc[penNum] = state->screenX;
    yLoc[penNum] = state->screenY;

    // Handle dialogues. Basically, pretend to be lots of mice
    if( fileDialogue->isVisible() )
    {
        QRect fileGeom = fileDialogue->geometry();
        if( fileGeom.contains(state->screenX,state->screenY) )
        {
            int  csrX = state->screenX - fileGeom.x();
            int  csrY = state->screenY - fileGeom.y();

            // Ensure the pen is drawn
            dialogCursor[penNum]->updatePenColour(annotations->getPenColour(penNum));
            dialogCursor[penNum]->moveCentre(csrX, csrY);
            dialogCursor[penNum]->show();

            QWidget *child = fileDialogue->childAt(csrX,csrY);

            QPoint pt = QPoint(state->screenX, state->screenY);

            if( child != NULL )
            {
                // Add mouse move events
                QMouseEvent ev(QEvent::MouseMove,
                               child->mapFromGlobal(pt),
                               Qt::NoButton,Qt::NoButton,Qt::NoModifier);
                QApplication::sendEvent(child,&ev);

                // Map pen events to mouse style events
                if( buttons.leftMouseAction == CLICKED )
                {
                    child->setFocus(Qt::MouseFocusReason);

                    ev = QMouseEvent(QEvent::MouseButtonPress,
                                     child->mapFromGlobal(pt),
                                     Qt::LeftButton,Qt::LeftButton,Qt::NoModifier);
                    QApplication::sendEvent(child,&ev);
                }
                else if(  buttons.leftMouseAction == RELEASED )
                {
                    child->setFocus(Qt::MouseFocusReason);

                    ev = QMouseEvent(QEvent::MouseButtonRelease,
                                     child->mapFromGlobal(pt),
                                     Qt::LeftButton,Qt::LeftButton,Qt::NoModifier);
                    QApplication::sendEvent(child,&ev);
                }

                if( buttons.rightMouseAction == CLICKED )
                {
                    child->setFocus(Qt::MouseFocusReason);

                    ev = QMouseEvent(QEvent::MouseButtonPress,
                                     child->mapFromGlobal(pt),
                                     Qt::RightButton,Qt::RightButton,Qt::NoModifier);
                    QApplication::sendEvent(child,&ev);
                }
                else if( buttons.rightMouseAction == RELEASED )
                {
                    child->setFocus(Qt::MouseFocusReason);

                    ev = QMouseEvent(QEvent::MouseButtonRelease,
                                     child->mapFromGlobal(pt),
                                     Qt::RightButton,Qt::RightButton,Qt::NoModifier);
                    QApplication::sendEvent(child,&ev);
                }
            }

            // Press used on dialogue, don't use it any more
            return;
        }
        else
        {
            dialogCursor[penNum]->hide();
        }
    }

    if( remoteSelector != NULL )
    {
        // TODO: Will pretend to be lots of mice for these dialogues too. Some day soon.
        // Currently only used for remote connection IP entry.
    }

    // Apply any events to the stickies
    if( sessionType == PAGE_TYPE_STICKIES )
    {
        // Adding new sticky notes
        if( addingNewStickyMode[penNum] )
        {
            if( buttons.leftMouseAction == CLICKED )
            {
                if( addNewSticky(penNum,cursor[penNum]->getCurrentPenSize(),QPoint(state->screenX,state->screenY)) )
                {
                    cursor[penNum]->removeStickyAddBrush();
                    addingNewStickyMode[penNum] = false;

                    waitForPenButtonsRelease[penNum] = true;
                }
            }
            else if( buttons.rightMouseAction == CLICKED )
            {
                // Cancel sticky add operation
                cursor[penNum]->removeStickyAddBrush();
                addingNewStickyMode[penNum] = false;

                waitForPenButtonsRelease[penNum] = true;
            }
            else if( GENERIC_ZOOM_IN(buttons) )
            {
                newStickySize[penNum] = 12*newStickySize[penNum]/10;
                cursor[penNum]->setStickyAddBrush(newStickySize[penNum], Qt::yellow);
            }
            else if( GENERIC_ZOOM_OUT(buttons) && newStickySize[penNum]>50 )
            {
                newStickySize[penNum] =  8*newStickySize[penNum]/10;
                cursor[penNum]->setStickyAddBrush(newStickySize[penNum], Qt::yellow);
            }

            cursor[penNum]->moveCentre(state->screenX,state->screenY);

            penIsDrawing[penNum] = false;

            return;
        }

        if( deleteStickyOnClick[penNum] )
        {
            for(int sn=0; sn<notes.count(); sn++)
            {
                if( notes.at(sn)->screenRect().contains(state->screenX,state->screenY) &&
                    buttons.leftMouseAction == CLICKED    )
                {
                    for(int penCheck=0; penCheck<MAX_SCREEN_PENS; penCheck ++)
                    {
                        // Can we change notes[sn] ?
                        if(    penNum   !=   penCheck                     &&
                            stickyBeingUpdatedByPenNum[penNum] == notes[sn] )
                        {
                            qDebug() << "Can't delete sticky being dragged by someone else.";
                            return;
                        }
                    }

                    // Remove the sticky
                    notes[sn]->hide();
                    notes.removeAt(sn);

                    deleteStickyOnClick[penNum] = false;
                    cursor[penNum]->setdefaultCursor();

                    annotations->deleteStickyNote(sn, penNum);

                    update();

                    return;
                }

            }
        }
        else if( startMoveOnStickyClick[penNum] )
        {
            if( buttons.leftMouseAction == CLICKED )
            {
                for(int sn=0; sn<notes.count(); sn++)
                {
                    if( notes.at(sn)->screenRect().contains(state->screenX,state->screenY) )
                    {
                        penIsDraggingSticky[penNum]    = true;
                        startMoveOnStickyClick[penNum] = false;
                        dragStickyEndOnRelease[penNum] = false;

                        QPoint pos = QPoint(state->screenX,state->screenY);

                        notes[sn]->setStickyDragStart( pos );
                        stickyBeingUpdatedByPenNum[penNum] = notes[sn];

                        stickyBeingUpdatedByPenNum[penNum]->raise();

                        return;
                    }
                }
            }
        }
        else if( penIsDraggingSticky[penNum] )
        {
            QPoint pos = QPoint(state->screenX,state->screenY);

            stickyBeingUpdatedByPenNum[penNum]->dragToo(pos);

            stickyBeingUpdatedByPenNum[penNum]->raise();

            if( dragStickyEndOnRelease[penNum] )
            {
                if( buttons.leftMouse == false )
                {
                    int stickyNum = notes.indexOf(stickyBeingUpdatedByPenNum[penNum]);

                    if( stickyNum>=0 )
                    {
                        annotations->moveStickyNote(stickyBeingUpdatedByPenNum[penNum]->pos(),stickyNum, penNum);
                    }

                    cursor[penNum]->setdefaultCursor();
                    cursor[penNum]->raise();

                    penIsDraggingSticky[penNum]        = false;
                    stickyBeingUpdatedByPenNum[penNum] = NULL;

                    waitForPenButtonsRelease[penNum] = true;
                }
            }
            else    // End on clicking by left or right buttons
            {
                if( buttons.leftMouseAction == CLICKED || buttons.rightMouseAction == CLICKED )
                {
                    int stickyNum = notes.indexOf(stickyBeingUpdatedByPenNum[penNum]);

                    cursor[penNum]->setdefaultCursor();

                    if( stickyNum>=0 )
                    {
                        annotations->moveStickyNote(stickyBeingUpdatedByPenNum[penNum]->pos(),stickyNum,penNum);
                    }

                    cursor[penNum]->raise();

                    penIsDraggingSticky[penNum]        = false;
                    stickyBeingUpdatedByPenNum[penNum] = NULL;
                    waitForPenButtonsRelease[penNum]   = true;
                }
            }

            return;
        }
        else
        {
            for(int sn=0; sn<notes.count(); sn++)
            {
                if( notes.at(sn)->screenRect().contains(state->screenX,state->screenY) )
                {
                    cursor[penNum]->moveCentre(state->screenX,state->screenY);

                    QPoint clickPos = QPoint(state->screenX,state->screenY) - notes[sn]->pos();

                    if( clickPos.x() < 0 || clickPos.y() >= notes[sn]->width()     ||
                            clickPos.y() < 0 || clickPos.y() >= notes[sn]->height()   )
                    {
                        qDebug() << "Internal error: internal clickpos:" << clickPos;
                        return;
                    }

                    if( notes.at(sn)->clickIsInDragBar(clickPos) )
                    {
                        stickyBeingUpdatedByPenNum[penNum] = notes[sn];

                        if( buttons.leftMouseAction == CLICKED )
                        {
                            penIsDraggingSticky[penNum]        = true;
                            dragStickyEndOnRelease[penNum]     = true;

                            cursor[penNum]->setMoveBrush();

                            notes[sn]->setStickyDragStart( QPoint(state->screenX,state->screenY) );
                        }
                    }
                    else
                    {
                        stickyBeingUpdatedByPenNum[penNum] = notes[sn];

                        notes.at(sn)->applyPenData(penNum, clickPos, buttons);

                        penIsDrawing[penNum] = false;
                    }

                    cursor[penNum]->moveCentre(state->screenX,state->screenY);
                    return;
                }
            }
        }
    }
    else // PAGE_TYPE_OVERLAY && PAGE_TYPE_WHITEBOARD
    {
        if( pageSelectActive &&
            (buttons.rightMouseAction == CLICKED || buttons.leftMouseAction == CLICKED) )
        {
            int   pageStartX = titlePos.x()+titleImage->width();
            QRect bounds(pageStartX,titlePos.y(),
                         pageSelectImage->width(), pageSelectImage->height());

            if( bounds.contains(state->screenX,state->screenY) )
            {
                int opt = (state->screenX - pageStartX + entryWidth/2)/entryWidth;
                qDebug() << "Select: opt=" << opt;
                if( opt == 0 )
                {
                    if( firstShownThumbNail > 0 )
                    {
                        // TODO: scrolling options + start arrow check
                    }
                }
                // else if ( ... ) TODO: scrolling options: end arrow check
                else if( opt>=1 )
                {
                    // First ensure we've updated the current thumbnail
                    if( currentPageNumber >= ((int)(pagePreview.size() )) )
                    {
                        // Not been cached yet. Add it.
                        pagePreview.push_back( annotationsImage->scaledToWidth(previewWidth) );
                    }
                    else
                    {
                        pagePreview[currentPageNumber] = annotationsImage->scaledToWidth(previewWidth);
                    }

                    if( opt>=(1 + pagePreview.size()))
                    {
                        // New page.
                        newPage();
                    }
                    else
                    {
                        // Go to option page
                        annotations->gotoPage(opt-1);
                        replay->gotoPage(opt-1);

                        currentPageNumber = opt-1;
                    }
                }
                pageSelectActive = false;
                update();

                // Mask out button presses until released.
                waitForPenButtonsRelease[penNum] = true;
            }
            else
            {
                qDebug() << "Point " << state->screenX << state->screenY << "out of page select bounds" <<
                           bounds;

                pageSelectActive = false;
                update();
            }

            // Press used on dialogue, don't use it any more
            return;
        }
    }

    if( deleteStickyOnClick[penNum]           &&
        ( buttons.leftMouseAction == CLICKED )  )
    {
        // Used it (even if it was clicked not on a sticky)
         deleteStickyOnClick[penNum] = false;

         cursor[penNum]->setdefaultCursor();
    }

    if( playingRecordedSession && buttons.leftMouseAction == CLICKED )
    {
        if( jogShuttlePos.contains(state->screenX,state->screenY) )
        {
            int opt = ( state->screenX - jogShuttlePos.x() ) * 3 / jogShuttlePos.width();

            switch( opt )
            {
            case 0:  playbackRewind();
                     break;
            case 1:  playbackStop();
                     break;
            case 2:  playbackTogglePlayPause();
                     break;
            default: qDebug() << "Unexpected jog shuttle option: " << opt;
            }
        }

        // Press used on dialogue, don't use it any more
        return;
    }

    // Let's not try and annotate the recorded annotations for now.
    if( ! playingRecordedSession )
    {
        if( penInTextMode[penNum] )
        {
            if( buttons.leftMouse == CLICKED )
            {
                setTextPosFromPixelPos(penNum, state->screenX, state->screenY);

                penIsDrawing[penNum]             = false;
                waitForPenButtonsRelease[penNum] = true;

                return;
            }
        }

        if( ! brushAction[penNum] ) // Pen menu (but want select to stop brush action)
        {
            if( penMenu[penNum]->isVisible() )
            {
                if( ! penMenu[penNum]->updateWithInput(state->screenX, state->screenY, &buttons) )
                {
                    // false is returned if the menu is no longer present. It is now
                    // that we want to mask actions until all buttons have been released.
                    waitForPenButtonsRelease[penNum] = true;
                }

                penIsDrawing[penNum] = false;
                selectButton         = false;

                return; // This prevents the cursor from moving in menu mode
            }
            else if( buttons.rightMouseAction == CLICKED )
            {
                qDebug() << "Start a pen menu.";

                penMenu[penNum]->resetMenu();
                penMenu[penNum]->moveCentre(state->screenX, state->screenY);
                penMenu[penNum]->show();

                penIsDrawing[penNum] = false;
                selectButton         = false;
            }
            else if( buttons.centreMouse )
            {
                int currentThickness = annotations->getPenThickness(penNum);

                if( buttons.scrollMinusAction == CLICKED && currentThickness>1 )
                {
                    if( currentThickness<5 )
                    {
                        if( currentThickness>1 ) currentThickness --;
                    }
                    else
                    {
                        currentThickness = currentThickness * 4 / 5;
                    }

                    qDebug() << "Zoom pen thickness down to" << currentThickness;

                    annotations->setPenThickness(penNum,currentThickness);
                    replay->setPenThickness(penNum,currentThickness);
                    cursor[penNum]->updatePenSize(currentThickness);
                }
                else if( buttons.scrollPlusAction == CLICKED && currentThickness<200 )
                {
                    if( currentThickness<5 )
                    {
                        currentThickness ++;
                    }
                    else
                    {
                        currentThickness = currentThickness * 5 / 4;
                    }

                    qDebug() << "Zoom pen thickness up to " << currentThickness;

                    annotations->setPenThickness(penNum,currentThickness);
                    replay->setPenThickness(penNum,currentThickness);
                    cursor[penNum]->updatePenSize(currentThickness);
                }
            }
        }

        // Main overlay/screen dawing operations (stickies do this themselves)
        if( sessionType != PAGE_TYPE_STICKIES )
        {
            if( brushAction[penNum] )
            {
                // If left click, stamp an image; if right click, exit brush mode

                if( buttons.rightMouseAction == CLICKED )
                {
                    qDebug() << "Deselected brush mode with right mouse.";

                    brushAction[penNum] = false;
                    cursor[penNum]->setdefaultCursor();
                }
                else if( selectButton && ! wasSelectButton )
                {
                    qDebug() << "Brush paint.";

                    annotations->CurrentBrushAction(penNum,xLoc[penNum],yLoc[penNum]);
                    replay->repeatPenAction(penNum,xLoc[penNum],yLoc[penNum]);

                    waitForPenButtonsRelease[penNum] = true;
                    return;
                }

                // Allow for enlarge/diminish with the MODE+SCROLL_UP or MODE+SCROLL_DOWN

                if( buttons.centreMouse )
                {
                    if( buttons.scrollMinusAction == CLICKED )
                    {
                        annotations->zoomBrush(penNum, -10);
                        replay->zoomBrush(penNum, -10);

                        cursor[penNum]->setCursorFromBrush( annotations->getBrushPoints(penNum) );
                    }
                    else if( buttons.scrollPlusAction == CLICKED )
                    {
                        annotations->zoomBrush(penNum, +10);
                        replay->zoomBrush(penNum, +10);

                        cursor[penNum]->setCursorFromBrush( annotations->getBrushPoints(penNum) );
                    }
                }
            }
            else if( ! penIsDrawing[penNum] && selectButton )
            {
                annotations->startPenDraw(penNum);
                replay->startPenDraw(penNum);
            }
            else if( penIsDrawing[penNum] && ! selectButton )
            {
                annotations->stopPenDraw(penNum);
                replay->stopPenDraw(penNum);
            }
        }
    }

    // TODO: offset and cursor size should be from same data source
    cursor[penNum]->moveCentre(state->screenX,state->screenY);

    penIsDrawing[penNum] = selectButton;
}



#define DEBUG_SAMPLE_RATE

#ifdef DEBUG_SAMPLE_RATE
static int   rateTest = 0;
static QTime lastReadPenTime;
static bool  haveLastReadTime = false;
#endif

// This does the API call to read each pen's state
void overlayWindow::readPenData(void)
{
    pen_state_t state;
    bool        changed = false;
    int         xLoc[MAX_SCREEN_PENS];
    int         yLoc[MAX_SCREEN_PENS];

#if defined(Q_OS_WIN)
    // Try to always be on top (for overlay only, not opaque).
    if( ! useMouseAsInputToo ) raise();
#endif

    readPenDataMutex.lock();

#ifdef DEBUG_SAMPLE_RATE
    // Debug the sample rate
    rateTest ++;
    if( rateTest >= 200 )
    {
        QTime now = QTime::currentTime();
        if( haveLastReadTime )
        {
//            qDebug() << "readPenData rate:" << 200.0/( 0.001*lastReadPenTime.elapsed() ) << "Hz";
            lastReadPenTime.restart();
        }
        else
        {
            lastReadPenTime.start();
            haveLastReadTime = true;
        }
        rateTest = 0;
    }
#endif

    if( ! remoteConnection )
    {
        for(int penNum=0; penNum<MAX_SCREEN_PENS; penNum++)
        {
            // Reate pen "state" from sysTray or construct data from local mouse
            getPenData(penNum, state);

            // /////////////////////////////////////////
            // Handle drawing pens starting or stopping
            if( COS_PEN_IS_ACTIVE(state) && ! penWasActive[penNum] )
            {
                // Retreive the pen settings (name, colour, etc)
                if( penNum != LOCAL_MOUSE_PEN )
                {
                    if( COS_TRUE == cos_retreive_pen_settings(penNum,&(penSettings[penNum])) )
                    {
                        QColor newPenColour(penSettings[penNum].colour[0],
                                            penSettings[penNum].colour[1],
                                            penSettings[penNum].colour[2]);

                        cursor[penNum]->updatePenColour(newPenColour);
                        penMenu[penNum]->setColour(newPenColour);

                        annotations->setPenColour(penNum, newPenColour);
                        replay->setPenColour(penNum, newPenColour);
                    }
                    else
                    {
                        penSettings[penNum].users_name[0] = (char)0;
                    }
                }

                lastButtons[penNum] = 0;

                if( COS_GET_PEN_MODE(state) == COS_MODE_NORMAL )
                {
                    // Pen has become available, so let's show it
                    cursor[penNum]->show();
                }

                penLastMode[penNum] = COS_GET_PEN_MODE(state);
            }
            else if( penWasActive[penNum] && ! COS_PEN_IS_ACTIVE(state) )
            {
                cursor[penNum]->hide();
                penMenu[penNum]->hide();
            }

            // ////////////////////////
            // Handle Presentation pens
            if( COS_PEN_IS_ACTIVE(state) )
            {
                switch( COS_GET_PEN_MODE(state) )
                {
                case COS_MODE_DRIVE_PRESENTATION:

                    if( penLastMode[penNum] == COS_MODE_DRIVE_PRESENTATION )
                    {
                        // Existing presentation pen
                        if( lastAppCtrlSequenceNum[penNum] != state.app_ctrl_sequence_num )
                        {
                            // Note that if the acrual presentation doesn't advance,
                            // then we will get out of sync
                            switch( state.app_ctrl_action )
                            {
                            case APP_CTRL_PREV_PAGE:

                                prevPage();
                                changed = true;
                                break;

                            case APP_CTRL_NEXT_PAGE:
                            case APP_CTRL_START_PRESENTATION:
                            case APP_CTRL_EXIT_PRESENTATION:

                                // Start a presentation with a new page
                                nextPage();
                                changed = true;
                                break;

                            case APP_CTRL_SHOW_HIGHLIGHT:

                                // toggle "laser pointer"
                                cursor[penNum]->togglePresentationPointer();
                                changed = true;
                                break;

                            case APP_CTRL_NO_ACTION:
                            default:

                                break;
                            }

                            lastAppCtrlSequenceNum[penNum] = state.app_ctrl_sequence_num;
                        }
                    }
                    else
                    {
                        qDebug() << "Pen" << penNum << "has moved into presentation mode.";

                        cursor[penNum]->setPresentationMode();
//                        cursor[penNum]->show();
                        penMenu[penNum]->hide();

                        // New presentation pen
                        lastAppCtrlSequenceNum[penNum] = state.app_ctrl_sequence_num;
                        changed                        = true;
                    }

                    cursor[penNum]->moveCentre(state.screenX,state.screenY);

                    break;

                case COS_MODE_DRIVE_MOUSE:

                    if( penLastMode[penNum] != COS_MODE_DRIVE_MOUSE )
                    {
                        cursor[penNum]->hide();
                        penMenu[penNum]->hide();
                    }

                    break;

                case COS_MODE_NORMAL:

                    if( penLastMode[penNum] != COS_MODE_NORMAL )
                    {
                        qDebug() << "Pen" << penNum << "has moved into overlay mode.";

                        // TODO: are the state variables corresponding to this updated?
                        cursor[penNum]->setOverlayMode();
                        cursor[penNum]->show();

                        // Lost presentation pen
                        lastAppCtrlSequenceNum[penNum] = state.app_ctrl_sequence_num;
                    }
                    else
                    {
                        if( lastAppCtrlSequenceNum[penNum] != state.app_ctrl_sequence_num )
                        {
                            if( APP_CTRL_IS_RAW_KEYPRESS(state.app_ctrl_action) )
                            {
                                addTextCharacterToOverlay(penNum,state.app_ctrl_action);
                            }
                            else if( APP_CTRL_IS_MENU_COMMAND(state.app_ctrl_action) )
                            {
                                applyReceivedMenuCommand(penNum,state.app_ctrl_action);
                            }
                            else
                            {
                                qDebug() << "Unrecognised app_ctrl message type:"
                                         << QString("0x%1").arg(state.app_ctrl_action,0,16)
                                         << "Seq num"
                                         << QString("0x%1").arg(state.app_ctrl_sequence_num,0,16);
                            }
                            lastAppCtrlSequenceNum[penNum] = state.app_ctrl_sequence_num;
                        }
                    }

                    break;
                }

                penLastMode[penNum] = COS_GET_PEN_MODE(state);
            }

            // //////////////////////////////////
            // If we have an active pen, do stuff
            if( COS_PEN_IS_ACTIVE(state)       &&
                COS_GET_PEN_MODE(state) == COS_MODE_NORMAL )
            {
                activePenUpdate(&state,penNum,xLoc,yLoc);

                changed = true;
            }

            penWasActive[penNum] = COS_PEN_IS_ACTIVE(state) /* &&
                                   COS_GET_PEN_MODE(state) == COS_MODE_NORMAL; */;
        } // for each pen

        if( sessionType == PAGE_TYPE_STICKIES )
        {
            bool  fakePresent[MAX_SCREEN_PENS];
            int   fakeX[MAX_SCREEN_PENS];
            int   fakeY[MAX_SCREEN_PENS];

            for(int sn=0; sn<notes.size(); sn++ )
            {
                for(int p=0; p<MAX_SCREEN_PENS; p++)
                {
                    if( ! penWasActive[p] || COS_GET_PEN_MODE(state) != COS_MODE_NORMAL )
                    {
                        fakePresent[p] = false;
                    }
                    else
                    {
                        if( notes[sn]->screenRect().contains(QPoint(xLoc[p], yLoc[p])) )
                        {
                            fakePresent[p] = true;
                            fakeX[p]       = xLoc[p] - notes[sn]->x();
                            fakeY[p]       = yLoc[p] - notes[sn]->y();
                        }
                        else
                        {
                            fakePresent[p] = false;
                        }
                    }
                }

                notes[sn]->penPositions(fakePresent, fakeX, fakeY);
            }
        }
        else
        {
            annotations->penPositions(penWasActive,xLoc,yLoc);
            replay->penPositions(penWasActive,xLoc,yLoc);
        }

        if( changed )
        {
            repaint();
        }
    }
    else    // Remote connection
    {
        getPenData(penConnectedToRemote, state);
        bool buttonPress  = (state.buttons & BUTTON_LEFT_MOUSE) != 0;
        bool pButtonPress = (state.buttons & BUTTON_LEFT_MOUSE) != 0;
#ifdef USE_BUTTON_STRIP
        if(menuButtons->getMenuMode() != overlayMode) buttonPress = false; // No trace in view, mouse or presentation modes
#endif
#ifdef USE_THREE_BUTTON_UI
        if(threeButtonInterface->getMenuMode() != (overlayMode)) buttonPress  = false; // No trace in view, mouse or presentation modes
        if(threeButtonInterface->getMenuMode() != (privateMode)) pButtonPress = false;
        //if(threeButtonInterface->getMenuMode() == privateMode) buttonPress = true; // Lazy man's fix!*/
#endif
        // No packets if not connected
        if( threeButtonInterface->getMenuMode() == (overlayMode) )
        {
            drawTrace->updateLocalDrawCache(state.screenX, state.screenY, buttonPress);
            sendConnectedPenDataToHost(state);
        }
        if( threeButtonInterface->getMenuMode() == (privateMode) )
        {
            drawTrace->updatePrivateLocalDrawCache(state.screenX, state.screenY, pButtonPress);
            sendConnectedPenDataToHost(state);
        }
    }

    readPenDataMutex.unlock();
}


void overlayWindow::setPenMode(int penNum, int newMode)
{
    switch( newMode )
    {
    case APP_CTRL_PEN_MODE_LOCAL:       qDebug() << "Send LOCAL pen mode command.";        break;
    case APP_CTRL_PEN_MODE_OVERLAY:     qDebug() << "Send OVERLAY pen mode command.";      break;
    case APP_CTRL_PEN_MODE_PRESNTATION: qDebug() << "Send PRESENTATION pen mode command."; break;
    case APP_CTRL_PEN_MODE_MOUSE:       qDebug() << "Send MOUSE pen mode command.";        break;
    default:                            qDebug() << "stePenMode() bad pen mode value:" << newMode;
    }

    modeSetCommand = APP_CTRL_PACKAGE_PENMODE(newMode);
    modeSendRepeat = 10;
}


void overlayWindow::setMouseButtonPulse(int penNum, fakeMouseButtonAction_t action)
{

    switch( action )
    {
    case MOUSE_LEFT:       mouseButtonsPulse |= BUTTON_LEFT_MOUSE;   break;
    case MOUSE_MIDDLE:     mouseButtonsPulse |= BUTTON_MIDDLE_MOUSE; break;
    case MOUSE_RIGHT:      mouseButtonsPulse |= BUTTON_RIGHT_MOUSE;  break;
    case MOUSE_SCROLLUP:   mouseButtonsPulse |= BUTTON_SCROLL_UP;    break;
    case MOUSE_SCROLLDOWN: mouseButtonsPulse |= BUTTON_SCROLL_DOWN;  break;

    default:    // unregognised
        return;
    }

    mouseButtonsPulseCount = 8;
}


void overlayWindow::sendPresntationControl(int penNum, presentation_controls_t action)
{
    switch( action )
    {
    case NEXT_SLIDE:     presentationCommand = APP_CTRL_NEXT_PAGE;           break;
    case PREV_SLIDE:     presentationCommand = APP_CTRL_PREV_PAGE;           break;
    case SHOW_HIGHLIGHT: presentationCommand = APP_CTRL_SHOW_HIGHLIGHT;      break;
    case START_SHOW:     presentationCommand = APP_CTRL_START_PRESENTATION;  break;
    case STOP_SHOW:      presentationCommand = APP_CTRL_EXIT_PRESENTATION;   break;
    case START_DEFAULT:  presentationCommand = APP_CTRL_START_SHORTCUT;      break;

    default:  return;
    }

    presentationSequenceNumber ++;
}


void overlayWindow::getPenData(int penNum, pen_state_t &state)
{
    // ////////////////////////////////////////////////////////////
    // Get data from local mouse, and (if running) the sysTray app.
    if( penNum == LOCAL_MOUSE_PEN )
    {
        if( ! useMouseAsInputToo )
        {
            COS_PEN_SET_ALL_FLAGS(state,COS_PEN_INACTIVE);
        }
        else
        {
            // Hack: sometimes we miss the release event so never lose this flag
            if( waitForPenButtonsRelease[LOCAL_MOUSE_PEN] &&
                Qt::NoButton == QApplication::mouseButtons() )
            {
                mouseButtons = 0;
            }

            COS_PEN_SET_ALL_FLAGS(state,COS_PEN_ACTIVE);

            // Scale to match zoomed image (tablet & remote PC) scaling to [0:10000,0:10000] is in tx (sendConnectedPenToHost)
            int XRemoteCentre = (mouseX - (width/2))  * nonLocalViewScale * annotationsImage->width() / width;
            int YRemoteCentre = (mouseY - (height/2)) * nonLocalViewScale * annotationsImage->height() / height;

            state.screenX = (XRemoteCentre + nonLocalViewCentreX);
            state.screenY = (YRemoteCentre + nonLocalViewCentreY);

            state.buttons = mouseButtons;

            if( mouseButtons & (BUTTON_SCROLL_UP|BUTTON_SCROLL_DOWN) )
            {
                wheelPulseLength --;
                if( wheelPulseLength<1 )
                {
                    mouseButtons &= ~(BUTTON_SCROLL_UP|BUTTON_SCROLL_DOWN);
                }
            }

            // As we convert rotations to +/- pulses for scroll wheel, clear +/-
            // event after use.

            mouseButtons &= ~(BUTTON_SCROLL_UP|BUTTON_SCROLL_DOWN);

            // Overwrite mouse buttons in output (except left mouse) if there was a
            // menu command to do a pulse mouse button.
            if( mouseButtonsPulseCount > 0 )
            {
                state.buttons = (mouseButtonsPulse & ~BUTTON_LEFT_MOUSE) |
                                (mouseButtons      & BUTTON_LEFT_MOUSE);

                mouseButtonsPulseCount --;
            }

            // And use local keypresses too
            if( lastKeypressSequenceNumber != keypressSequenceNumber )
            {
//              state.app_ctrl_action       = keyPressModifiers | APP_CTRL_RAW_KEYPRESS | keyPress;
                state.app_ctrl_action       = APP_CTRL_RAW_KEYPRESS | keyPress;
                state.app_ctrl_sequence_num = sendSequenceNumber;

                sendSequenceNumber ++;

                qDebug() << "Create local keypress:"
                         << QString("0x%1").arg(state.app_ctrl_action,0,16);

                lastKeypressSequenceNumber = keypressSequenceNumber;
            }
            else if( lastMenuSequenceNumber != menuSequenceNumber )
            {
                state.app_ctrl_action       = APP_CTRL_PACKAGE_MENUCMD(menuCommand);
                state.app_ctrl_sequence_num = sendSequenceNumber;

                sendSequenceNumber ++;

                qDebug() << "Create menu command:"
                         << QString("0x%1").arg(state.app_ctrl_action,0,16);

                lastMenuSequenceNumber = menuSequenceNumber;
            }
            else if( lastPresentationSequenceNumber != presentationSequenceNumber )
            {
                state.app_ctrl_action       = presentationCommand;
                state.app_ctrl_sequence_num = sendSequenceNumber;

                sendSequenceNumber ++;

                qDebug() << "Create menu command:"
                         << QString("0x%1").arg(state.app_ctrl_action,0,16);

                lastPresentationSequenceNumber = presentationSequenceNumber;
            }
#ifdef TABLET_MODE
            else if( lastSendCharRepeat > 0 )
            {
                state.app_ctrl_action       = keyPress | APP_CTRL_RAW_KEYPRESS;
                state.app_ctrl_sequence_num = sendSequenceNumber;

                lastSendCharRepeat --;
            }
            else if( lastSendCharRepeat == 0 && sendCharacters.length() > 0 )
            {
                sendSequenceNumber ++;
                keyPress                    = sendCharacters[0];
                state.app_ctrl_action       = keyPress | APP_CTRL_RAW_KEYPRESS;
                state.app_ctrl_sequence_num = sendSequenceNumber;
                sendCharacters.remove(0);

                qDebug() << "Send key" << QString("0x%1").arg(state.app_ctrl_action,0,16);

                lastSendCharRepeat = NUM_CHAR_REPEATS;
            }
#endif
            else if( modeSendRepeat > 0 )
            {
                sendSequenceNumber ++;

                state.app_ctrl_action       = modeSetCommand;
                state.app_ctrl_sequence_num = sendSequenceNumber;

                modeSendRepeat --;
            }
            else
            {
                state.app_ctrl_action       = currentSendAppCommand;
                state.app_ctrl_sequence_num = sendSequenceNumber;
            }

            currentSendAppCommand = state.app_ctrl_action;
        }
    }
    else if( cos_API_is_running() == COS_TRUE )
    {
        if( COS_TRUE != cos_get_pen_data(penNum, &state) )
        {
            COS_PEN_SET_ALL_FLAGS(state,COS_PEN_INACTIVE);
        }
    }
    else
    {
        COS_PEN_SET_ALL_FLAGS(state,COS_PEN_INACTIVE);
    }
}

void overlayWindow::sendConnectedPenDataToHost(pen_state_t &state)
{
    int w = this->width;
    int h = this->height;
    int xLoc = 0;  // Arbritrary 0->10,000 on each axis (scaled back on receiver)
    int yLoc = 0;

    xLoc = state.screenX * 10000 / annotationsImage->width();  // Arbritrary 0->10,000 on each axis (scaled back on receiver)
    yLoc = state.screenY * 10000 / annotationsImage->height();
    dataPacket.msg.data.buttons = state.buttons;

    cursor[penConnectedToRemote]->show();
    cursor[penConnectedToRemote]->moveCentre(state.screenX,state.screenY);

    dataPacket.messageType      = NET_DATA_PACKET;
    dataPacket.messageTypeCheck = NET_DATA_PACKET;
    dataPacket.protocolVersion  = PROTOCOL_VERSION;

    dataPacket.msg.data.x[0] = (xLoc >> 8) & 255;
    dataPacket.msg.data.x[1] = (xLoc)      & 255;
    dataPacket.msg.data.y[0] = (yLoc >> 8) & 255;
    dataPacket.msg.data.y[1] = (yLoc)      & 255;

    // All keypressess data in the top byte is 0/1 (from observation)
//    qDebug() << "Send: app_ctrl" << QString("0x%1").arg(state.app_ctrl_action,0,16)
//                                 << QString("0x%1").arg(state.app_ctrl_sequence_num,0,16)
//             << "Pen num" << penConnectedToRemote;

    dataPacket.msg.data.app_ctrl_action[0] = ( state.app_ctrl_action >> 24 ) & 255; //((keyPress|keyPressModifiers) >> 24) & 0xFF ;
    dataPacket.msg.data.app_ctrl_action[1] = ( state.app_ctrl_action >> 16 ) & 255; //APP_CTRL_RAW_KEYPRESS >> 16;
    dataPacket.msg.data.app_ctrl_action[2] = ( state.app_ctrl_action >>  8 ) & 255; //(keyPress >> 8) & 0xFF;
    dataPacket.msg.data.app_ctrl_action[3] = ( state.app_ctrl_action       ) & 255; //(keyPress     ) & 0xFF;

    dataPacket.msg.data.app_ctrl_sequence_num[0] = (state.app_ctrl_sequence_num >> 24) & 0xFF;
    dataPacket.msg.data.app_ctrl_sequence_num[1] = (state.app_ctrl_sequence_num >> 16) & 0xFF;
    dataPacket.msg.data.app_ctrl_sequence_num[2] = (state.app_ctrl_sequence_num >>  8) & 0xFF;
    dataPacket.msg.data.app_ctrl_sequence_num[3] = (state.app_ctrl_sequence_num      ) & 0xFF;

    if(    (dataPacket.msg.data.buttons & BUTTON_LEFT_MOUSE)
        && (dataPacket.msg.data.buttons & BUTTON_RIGHT_MOUSE)  )
    {
        remoteConnection = false;
        showActionImage  = false;

        qDebug() << "DISCONNECT from remote system...";

        netClient->stopReceiveView();
        connectedToViewHost = false;
#ifndef TABLET_MODE
        if( ! sysTrayDetected )
        {
            screenGrab->resumeGrabbing();
            broadcast->resumeSending();
        }
#endif
        // Restore the last page view before the connection.
        annotationsImage->fill(Qt::transparent);
        highlights->fill(Qt::transparent);

        annotations->rebuildAnnotationsFromUndoList();

        if( useMouseAsInputToo )
        {
            setCursor(Qt::BlankCursor);
            annotations->setBackground(defaultOpaqueBackgroundColour);
            replayBackgroundImage->fill(defaultOpaqueBackgroundColour);

            // Give the mouse a initial state, as no events will have happened before first update
            mouseX       = QCursor::pos().x();
            mouseY       = QCursor::pos().y();
        }

        // For remote/replay views to allow scaling/zooming in
        nonLocalViewScale    = 1.0;
        nonLocalViewScaleMax = 1.0;
        nonLocalViewCentreX  = width/2;
        nonLocalViewCentreY  = height/2;

#ifdef TABLET_MODE
        // Kick off the connect to remote host dialogue as this is always a remote session
        remoteSelector->setStatusString(tr("Connect to a new session?"));
        remoteSelector->reStartRemoteConnection(width,height);
#endif

        qDebug() << "Drop remote connection as buttons =" << dataPacket.msg.data.buttons;
    }
    else    // Send data
    {
        if( remoteSelector->havePassword() )
        {
            remoteSelector->passwordEncryptMessage(&dataPacket);
        }

        remoteJoinSkt.writeDatagram((char *)&dataPacket,sizeof(dataPacket),hostAddr,hostPort);
        remoteJoinSkt.waitForBytesWritten();
    }
}

// Used by tablet app to switch between zoom/pan to overlay drawing modes

void overlayWindow::setZoomPanModeOn(bool on)
{
    localControlIsZoomPan = on;
}

// The following functions enable mouse mode

void overlayWindow::mouseMoveEvent(QMouseEvent *e)
{
//    mouseX = e->pos().x();
//    mouseY = e->pos().y();

    mousePressEvent(e);
}

void overlayWindow::mousePressEvent(QMouseEvent *e)
{
    mouseX = e->pos().x();
    mouseY = e->pos().y();

    int buttons         = e->buttons();
    int oldMouseButtons = mouseButtons;

#ifdef TABLET_MODE

    if( localControlIsZoomPan ) return;

#endif

    // Mouse buttons pressed
#ifndef Q_OS_MAC
    // For 3 button mouse:
    // Left,Right   = LEFT/RIGHT
    // Middle       = MIDDLE
    // Middle+Left  = OPT_A (mode switch):
    //                     need to press Middle, then Left to not draw accidentally
    // NMiddle+Left = MODE_SWITCH
    if( buttons & Qt::MidButton ) mouseButtons |= BUTTON_MIDDLE_MOUSE;
    else
    {
        mouseButtons &= ~(BUTTON_MIDDLE_MOUSE|BUTTON_OPT_A|BUTTON_MODE_SWITCH);
    }

    if( mouseButtons & BUTTON_MIDDLE_MOUSE )
    {
        if( buttons & Qt::LeftButton ) mouseButtons |= BUTTON_OPT_A;
        if( buttons & Qt::RightButton) mouseButtons |= BUTTON_MODE_SWITCH;
    }
    else
    {
        if( buttons & Qt::LeftButton )  mouseButtons |= BUTTON_LEFT_MOUSE;
        else                                 mouseButtons &= ~BUTTON_LEFT_MOUSE;
        if( buttons & Qt::RightButton ) mouseButtons |= BUTTON_RIGHT_MOUSE;
        else                                 mouseButtons &= ~BUTTON_RIGHT_MOUSE;
    }
#else
    if( buttons & Qt::LeftButton )  mouseButtons |= BUTTON_LEFT_MOUSE;
    else                                 mouseButtons &= ~BUTTON_LEFT_MOUSE;
    if( buttons & Qt::RightButton ) mouseButtons |= BUTTON_RIGHT_MOUSE;
    else                                 mouseButtons &= ~BUTTON_RIGHT_MOUSE;
    if( buttons & Qt::MidButton )
    {
        mouseButtons |= BUTTON_OPT_A;
        mouseButtons |= BUTTON_SCROLL_DOWN;
    }
    else
    {
        mouseButtons &= ~BUTTON_OPT_A;
        mouseButtons &= ~BUTTON_SCROLL_DOWN;
    }
#endif
    if( mouseButtons != 0 && mouseButtons != oldMouseButtons )
        qDebug() << "mouseButtons =" << QString("0x%1").arg(mouseButtons,2,16)
                 << "Buttons pressed:"
                 << (mouseButtons & BUTTON_LEFT_MOUSE   ? "Left":"")
                 << (mouseButtons & BUTTON_MIDDLE_MOUSE ? "Middle":"")
                 << (mouseButtons & BUTTON_RIGHT_MOUSE  ? "Right":"")
                 << (mouseButtons & BUTTON_OPT_A        ? "OptA":"")
                 << (mouseButtons & BUTTON_MODE_SWITCH  ? "Mode":"");
}

void overlayWindow::mouseReleaseEvent(QMouseEvent *e)
{
    mousePressEvent(e);

    return;

    mouseX = e->pos().x();
    mouseY = e->pos().y();

#ifdef TABLET_MODE

    if( localControlIsZoomPan ) return;

#endif

    // Mouse buttons pressed
    if( e->button() & Qt::LeftButton )  mouseButtons &= ~BUTTON_LEFT_MOUSE;
    if( e->button() & Qt::MidButton )   mouseButtons &= ~(BUTTON_OPT_A|BUTTON_MIDDLE_MOUSE);
    if( e->button() & Qt::RightButton ) mouseButtons &= ~(BUTTON_RIGHT_MOUSE|BUTTON_MODE_SWITCH);

    qDebug() << "Buttons out =" << QString("0x%1").arg(mouseButtons,2,16)
             << "Mouse buttons released:"
             << (e->button() & Qt::LeftButton  ? "Left":"")
             << (e->button() & Qt::MidButton   ? "Middle":"")
             << (e->button() & Qt::RightButton ? "Right":"");
}

void overlayWindow::wheelEvent(QWheelEvent *w)
{
    mouseX = w->pos().x();
    mouseY = w->pos().y();

#ifdef TABLET_MODE

    if( localControlIsZoomPan ) return;

#endif

    // Trigger event on 15 degrees or more (NB data here is in 1/8 degree)
    wheelDeltaAccumulate += w->delta();

    if( wheelDeltaAccumulate>= 14*8 )
    {
        mouseButtons        |= BUTTON_SCROLL_UP;
        mouseButtons        &= ~BUTTON_SCROLL_DOWN;   // Just to be sure ;)
        wheelDeltaAccumulate = 0;
        wheelPulseLength     = 4;  // Number of UDP packets sent with this button true
    }
    else if( wheelDeltaAccumulate<= -14*8 )
    {
        mouseButtons        |= BUTTON_SCROLL_DOWN;
        mouseButtons        &= ~BUTTON_SCROLL_UP;   // Just to be sure ;)
        wheelDeltaAccumulate = 0;
        wheelPulseLength     = 4;  // Number of UDP packets sent with this button true
    }

    qDebug() << "mouseButtons Scroll Up|Down" << ((mouseButtons & BUTTON_SCROLL_UP) != 0)
                                              << ((mouseButtons & BUTTON_SCROLL_DOWN) != 0);
}


#ifdef TABLET_MODE
void overlayWindow::pinchZoom(QPointF a, QPointF b, bool first)
{
    float  delX, delY;
    int    rmtViewWidthScaled  = (int)(annotationsImage->width() * nonLocalViewScale);
    int    rmtViewHeightScaled = (int)(annotationsImage->height() * nonLocalViewScale);

    // We can "miss" the start event if it only had one point. We handle "missing" the
    // end in the calling function (always set inPinchZoom to false on any event end).

    if( first || ! inPinchZoom )
    {
        pinchStartScale    = nonLocalViewScale;
        pinchStartCentreX  = nonLocalViewCentreX;
        pinchStartCentreY  = nonLocalViewCentreY;

        pinchStartMidpoint   = QPointF((a+b)/2);
        delX = a.x() - b.x();
        delY = a.y() - b.y();
        pinchStartSeparation = sqrt( delX*delX + delY*delY );
        inPinchZoom          = true;
        inTouchDrag          = false;

//        qDebug() << "Gesture start pos:" << pinchStartMidpoint << "distance" << pinchStartSeparation;
    }
    else
    {
        // Update the current view scale
        delX = a.x() - b.x();
        delY = a.y() - b.y();
        float newSeparation = sqrt( delX*delX + delY*delY );

        nonLocalViewScale = pinchStartScale * (pinchStartSeparation/newSeparation);

        // Update the current view center
        QPointF currentMidPointChange = QPointF((a+b)/2) - pinchStartMidpoint;
        nonLocalViewCentreX = pinchStartCentreX - pinchStartScale*currentMidPointChange.x()/nonLocalViewScale;
        nonLocalViewCentreY = pinchStartCentreY - pinchStartScale*currentMidPointChange.y()/nonLocalViewScale;

        // Make sure it's all valid
        normaliseZoomPan(rmtViewWidthScaled, rmtViewHeightScaled );

        update();
    }
}
#endif



// Allow zoom/pan with keyboard
//
// Keys:
// <ctrl>-[+/-]               Zoom in/out
//
// <ctrl>-[left/right arrows] Pan left/right
// <ctrl>-[up/down arrows]    pan up/down
//
// Pan distance is 10% of current view dimension

void overlayWindow::keyPressEvent(QKeyEvent *keyEv)
{
    if( ! keyEv || ! annotations ) return;

    bool zoomChanged             = false;
    int  rmtViewWidthScaled  = (int)(annotationsImage->width() * nonLocalViewScale);
    int  rmtViewHeightScaled = (int)(annotationsImage->height() * nonLocalViewScale);

    // Look for shift-escape key to quit
    if( keyEv->key() == Qt::Key_Escape                                 &&
        (keyEv->modifiers() & Qt::ShiftModifier ) == Qt::ShiftModifier    )
    {
        qDebug() << "Keyboard quit command.";

        mainMenuQuit();
        return;
    }

    // Reset zoom/center if not remote/replay/non-overlay. Do it later to ease development.
    if( ! playingRecordedSession && ! connectedToViewHost )
    {
        nonLocalViewScale    = 1.0;
        nonLocalViewScaleMax = 1.0;
        nonLocalViewCentreX  = width/2;
        nonLocalViewCentreY  = height/2;
    }

    // Look for recognised keys: need control + shift to do zoom / pan
    if( ( playingRecordedSession         ||       connectedToViewHost      ) &&
        ( (keyEv->modifiers() & Qt::ControlModifier ) == Qt::ControlModifier ||
          (keyEv->modifiers() & Qt::ShiftModifier )   == Qt::ShiftModifier    ) )
    {
        switch( keyEv->key() )
        {
        // Pan left/right/up/down

        case Qt::Key_Left:

            nonLocalViewCentreX -= rmtViewWidthScaled/10;
            zoomChanged = true;
            break;

        case Qt::Key_Right:

            nonLocalViewCentreX += rmtViewWidthScaled/10;
            zoomChanged = true;
            break;

        case Qt::Key_Up:

            nonLocalViewCentreY -= rmtViewHeightScaled/10;
            zoomChanged = true;
            break;

        case Qt::Key_Down:

            nonLocalViewCentreY += rmtViewHeightScaled/10;
            zoomChanged = true;
            break;

        // Zoom in/out

        case Qt::Key_Minus:

            nonLocalViewScale *= (11.0/10.0);
            zoomChanged = true;
            break;

        case Qt::Key_Plus:

            nonLocalViewScale *= (10.0/11.0);
            zoomChanged = true;
            break;

        default:

            // Store keypress for use (for connected sessions)
            keyPress = keyEv->key()|keyEv->modifiers();
            keypressSequenceNumber ++;
            break;
        }
    }
    else
    {
        // Store keypress for use
        keyPress = keyEv->key()|keyEv->modifiers();
        keypressSequenceNumber ++;
    }

    if( zoomChanged )
    {
        // Normalise view

        normaliseZoomPan(rmtViewWidthScaled, rmtViewHeightScaled);
    }
}

void overlayWindow::normaliseZoomPan(int rmtViewWidthScaled, int rmtViewHeightScaled)
{
    // TODO: Try to see whether using a # pixels calculation is worthwile.
    if( nonLocalViewScale < 0.1 )                    nonLocalViewScale = 0.1;
    if( nonLocalViewScale > nonLocalViewScaleMax  )  nonLocalViewScale = nonLocalViewScaleMax;

    int  rmtLeft   = nonLocalViewCentreX - annotationsImage->width()*nonLocalViewScale/2;
    int  rmtTop    = nonLocalViewCentreY - annotationsImage->height()*nonLocalViewScale/2;
    int  rmtRight  = rmtLeft + annotationsImage->width()*nonLocalViewScale;
    int  rmtBottom = rmtTop  + annotationsImage->height()*nonLocalViewScale;

    if( rmtLeft < 0 )
    {
        nonLocalViewCentreX = rmtViewWidthScaled/2;
    }
    else if( rmtRight > annotationsImage->width() )
    {
        nonLocalViewCentreX = (int)(annotationsImage->width()) - rmtViewWidthScaled/2;
    }

    if( rmtTop < 0 )
    {
        nonLocalViewCentreY = rmtViewHeightScaled/2;
    }
    else if( rmtBottom > annotationsImage->height() )
    {
        nonLocalViewCentreY = (int)(annotationsImage->height()) - rmtViewHeightScaled/2;
    }

//    qDebug() << "Zoom/pan - center: (" << nonLocalViewCentreX << nonLocalViewCentreY
//             << ") scale:" << nonLocalViewScale;

    // Schedule an update
    update();
}

void overlayWindow::keyReleaseEvent(QKeyEvent *keyEv)
{
    QWidget::keyReleaseEvent(keyEv);

    return; // May wish hold down to repeat in the future. Not yet.
}


#ifdef TABLET_MODE
bool overlayWindow::event(QEvent *e)
{
    // Check for tablet "Back Key"
    if( remoteConnection )
    {
        // Use back key as exit (i.e. trap it)
        if( e->type() == QEvent::KeyRelease )
        {
            QKeyEvent *kev = static_cast<QKeyEvent *>(e);

            if( kev->key() == Qt::Key_Back )
            {
                mainMenuDisconnect();

                return true;
            }
        }
    }
    else if( e->type() == QEvent::KeyRelease )
    {
        QKeyEvent *kev = static_cast<QKeyEvent *>(e);

        if( kev->key() == Qt::Key_Back )
        {
            mainMenuQuit();
        }
//        return QWidget::event(e);
    }

    if(e->type() == QEvent::TabletMove    || e->type() == QEvent::TabletPress &&
       e->type() == QEvent::TabletRelease )
    {
        // For tablet s-pen, but not seen these yet on a GalaxyTabPro... QTBUG-38379
        switch( e->type() )
        {
        case QEvent::TabletMove:    qDebug() << "TabletMove event";    break;
        case QEvent::TabletPress:   qDebug() << "TabletPress event";   break;
        case QEvent::TabletRelease: qDebug() << "TabletRelease event"; break;
        }

        return QWidget::event(e);
    }

    if( e->type() != QEvent::TouchBegin && e->type() != QEvent::TouchUpdate &&
        e->type() != QEvent::TouchEnd    )
    {
        return QWidget::event(e);
    }

    QTouchEvent *tev = static_cast<QTouchEvent *>(e);

    if( tev->touchPoints().size() < 1 ) return QWidget::event(e);

    // Check for events not over the display
#ifdef USE_BUTTON_STRIP
    if( menuButtons            &&  menuButtons->isVisible() )
#endif
#ifdef USE_THREE_BUTTON_UI
     if( threeButtonInterface  &&  threeButtonInterface->isVisible() )
#endif
    {
#ifdef USE_BUTTON_STRIP
        QRect menuButtonRect = QRect(menuButtons->pos(),menuButtons->size());
#endif
#ifdef USE_THREE_BUTTON_UI
        QRect menuButtonRect = QRect(threeButtonInterface->pos(),threeButtonInterface->size());
#endif
        if( menuButtonRect.contains(tev->touchPoints()[0].pos().toPoint()) )
        {
            qDebug() << "menuButtons touch event.";

            return QWidget::event(e);
        }
    }

    if( remoteSelector   &&   remoteSelector->isVisible()  )
    {
        QRect remoteConnectRect = QRect(remoteSelector->pos(),remoteSelector->size());

        if( remoteConnectRect.contains(tev->touchPoints()[0].pos().toPoint()) )
        {
            qDebug() << "remoteConnectDialogue touch event.";

            return QWidget::event(e);
        }
    }

//    qDebug() << "tev: @" << tev->touchPoints()[0].pos() << "menuButton rect" << menuButtons->rect();

    if( localControlIsZoomPan )
    {
        mouseButtons = 0;   // Defensive code

//        qDebug() << "Num touch points:" << tev->touchPoints().size();

        switch( e->type() )
        {
        case QEvent::TouchBegin:
        case QEvent::TouchUpdate:
        case QEvent::TouchEnd:

            switch( tev->touchPoints().size() )
            {
            case 1:   if( ! inTouchDrag )
                      {
                          touchDragStart    = tev->touchPoints()[0].pos();
                          pinchStartCentreX = nonLocalViewCentreX;
                          pinchStartCentreY = nonLocalViewCentreY;

                          inTouchDrag = true;
                          inPinchZoom = false;
                      }
                      else
                      {
                          QPointF deltaCenter = tev->touchPoints()[0].pos() - touchDragStart;
                          nonLocalViewCentreX = pinchStartCentreX - deltaCenter.x();
                          nonLocalViewCentreY = pinchStartCentreY - deltaCenter.y();
                      }

                      normaliseZoomPan( (int)(annotationsImage->width() * nonLocalViewScale),
                                        (int)(annotationsImage->height() * nonLocalViewScale) );

                      break;

            case 2:   pinchZoom( tev->touchPoints()[0].pos(), tev->touchPoints()[1].pos(),
                                 e->type() == QEvent::TouchBegin );
                      break;
            }

            if( e->type() == QEvent::TouchEnd )
            {
//                qDebug() << "Pinch zoom stop.";
                inPinchZoom = false;
                inTouchDrag = false;
            }

            return true;

        default:
            return QWidget::event(e);
        }
    }
    else
    {
        // Update mode
        switch( e->type() )
        {
        case QEvent::TouchBegin:
        case QEvent::TouchUpdate:
        case QEvent::TouchEnd:

            mouseX = tev->touchPoints()[0].pos().x();
            mouseY = tev->touchPoints()[0].pos().y();
#if 0
            switch( tev->touchPoints().size() )
            {
            case 1:   if( ! inTouchDrag )
                      {
                          touchDragStart    = tev->touchPoints()[0].pos();
                          pinchStartCentreX = nonLocalViewCentreX;
                          pinchStartCentreY = nonLocalViewCentreY;

                          inTouchDrag = true;
                          inPinchZoom = false;
                      }
                      else
                      {
                          QPointF deltaCenter = tev->touchPoints()[0].pos() - touchDragStart;
                          nonLocalViewCentreX = pinchStartCentreX - deltaCenter.x();
                          nonLocalViewCentreY = pinchStartCentreY - deltaCenter.y();
                      }

                      normaliseZoomPan( (int)(annotationsImage->width() * nonLocalViewScale),
                                        (int)(annotationsImage->height() * nonLocalViewScale) );

                      break;

            case 2:   pinchZoom( tev->touchPoints()[0].pos(), tev->touchPoints()[1].pos(),
                                 e->type() ==QEvent::TouchBegin );
                      break;
            }
#endif
            if( e->type() == QEvent::TouchEnd )
            {
                mouseButtons &= ~BUTTON_LEFT_MOUSE;
            }
            else
            {
//                qDebug() << "Touch at" << mouseX << mouseY;
                mouseButtons |= BUTTON_LEFT_MOUSE;
            }

            return true;

        default:
            return QWidget::event(e);
        }
    }
}

#endif



// Basically, we copy our background pixmap onscreen to paint
void overlayWindow::paintEvent(QPaintEvent *event)
{
    QPainter painter(this);

    if( showIntroBackground )
    {
        // Set a background colour to match the image background
        painter.setBrush(QColor(255,255,255));
        painter.drawRect(0,0,width,height);

        // Add the image
        QRectF imageTarget;

        if( width > height ) {
            imageTarget = QRectF((width-height)/2,0,height,height);
        } else {
            imageTarget = QRectF(0,(height-width)/2,width,width);
        }

        QImage  background = QImage(":/images/cosnetameetingpurple.jpg");
        QRectF  bounds     = QRectF(background.rect());

        painter.drawImage(imageTarget,background,bounds);

        // Add a text message

        painter.end();

        return;
    }

    // Add the overlay(s). NB Background image scaled to target size as use QRect form
    QRect target = QRect(0,0,width,height);
    QRect source = QRect(nonLocalViewCentreX-(replayBackgroundImage->width()*nonLocalViewScale/2),
                         nonLocalViewCentreY-(replayBackgroundImage->height()*nonLocalViewScale/2),
                                              replayBackgroundImage->width()*nonLocalViewScale,
                                              replayBackgroundImage->height()*nonLocalViewScale );

    if( playingRecordedSession || connectedToViewHost )
    {
//        qDebug() << "overlayWindow::paintEvent() - paint replayBackgroundImage";
        // Remote / recorded desktop (for transparent overlays)
#ifdef Q_OS_IOS
        QImage image = *replayBackgroundImage;
        painter.drawImage(target, image, source);
#else
        painter.drawImage(target, *(replayBackgroundImage), source);
#endif
//        qDebug() << "overlayWindow::paintEvent() - paint replayBackgroundImage done";
    }
#if 0
    source = QRect(nonLocalViewCentreX-(annotationsImage->width()*nonLocalViewScale/2),
                   nonLocalViewCentreY-(annotationsImage->height()*nonLocalViewScale/2),
                   annotationsImage->width()*nonLocalViewScale,
                   annotationsImage->height()*nonLocalViewScale );
#else
    if( annotationsImage->width() != replayBackgroundImage->width() )
    {
        qDebug() << "Width of annotations annotations and background don't match";
    }
    if( annotationsImage->height() != replayBackgroundImage->height() )
    {
        qDebug() << "Height of annotations annotations and background don't match";
    }
#endif
    if( ! connectedToViewHost )
    {
        qDebug() << "overlayWindow::paintEvent() - paint host stuff: annotationsImage, highlights & textLayerImage";

        painter.drawImage(target,*annotationsImage, source);
        painter.drawImage(target,*highlights,       source);
        painter.drawImage(target,*textLayerImage,   source);

        // Logo defaults to top left on top of overlay. (either REC>, PLAY> or RMT>)
        if( showActionImage ) painter.drawImage(titlePos,(*titleImage) );

        // Replay controls
        if( playingRecordedSession )
        {
            painter.drawImage(jogShuttlePos,*jogShuttleImage);
        }

        // Page select controls
        if( pageSelectActive )
        {
//            int x = showActionImage ? titlePos.x()+titleImage->width() : 0;
//            int y = showActionImage ? titlePos.y() : 0;
            painter.drawImage(titlePos.x(),titlePos.y(), *pageSelectImage);
        }
    }
    else
    {
        if( drawTrace->localDrawCachePresent() )
        {
            //qDebug() << "overlayWindow::paintEvent() - localDrawCachePresent() paint annotationsImage" << annotationsImage;
            painter.drawImage(target,*annotationsImage, source);
        }
        if( drawTrace->privateNoteDrawCachePresent() )
        {
            //qDebug() << "overlayWindow::paintEvent() - privateNoteDrawCashePresent() paint annotationsImage" << annotationsImage;
            painter.drawImage(target, *annotationsImage, source);
        }
    }

    painter.end();
}


QImage *overlayWindow::imageOfDrawnText( QString text )
{
    QFontMetrics metric(font());
    QSize        size = metric.size(Qt::TextSingleLine, text);
    QImage      *image = new QImage(size.width()+12, size.height()+12, QImage::Format_ARGB32_Premultiplied);
    QPainter     painter;

    image->fill(Qt::transparent);

    painter.begin(image);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setBrush(Qt::white);
    painter.drawRoundedRect(QRectF(0.5, 0.5, image->width()-1, image->height()-1),
                            25, 25, Qt::RelativeSize);

    painter.setBrush(Qt::black);
    painter.drawText(QRect(QPoint(6, 6), size), Qt::AlignCenter, text);
    painter.end();

    return image;
}


QImage *overlayWindow::imageOfJogShuttle(int screenWidth, bool showPause)
{
    int w = screenWidth/12;
    int h = w / 3;

    QImage *image = new QImage(w, h, QImage::Format_ARGB32_Premultiplied);

    image->fill(Qt::transparent);

    QPainter painter(image);
    painter.setRenderHint(QPainter::Antialiasing);

    // background
    painter.setBrush(defaultOpaqueBackgroundColour);

    int sz    = h*9/10;
    int arcSz = sz/5;

    for(int i=0; i<3; i++)
    {
        painter.drawRoundedRect( i*w/3,0, sz,sz, arcSz,arcSz );
    }

    // Draw symbols
    painter.setBrush(Qt::white);
    painter.setPen(Qt::white);
    // Return to start
    painter.drawRect(arcSz,arcSz, arcSz,sz-2*arcSz);
    QPointF leftAngle[3] = { QPointF(0+arcSz,h/2),
                             QPointF(sz-arcSz,arcSz),
                             QPointF(sz-arcSz,sz-arcSz)};
    painter.drawPolygon(leftAngle,3);
    // Stop
    painter.drawRect(arcSz+w/3,arcSz, sz-(2*arcSz),sz-(2*arcSz));
    // Play/pause
    if( showPause )
    {
        painter.drawRect(arcSz+w*2/3,arcSz, arcSz,sz-(2*arcSz));
        painter.drawRect(w-(3*arcSz),arcSz, arcSz,sz-(2*arcSz));
    }
    else
    {
        QPointF rightAngle[3] = {QPointF(w-arcSz,h/2),
                                 QPointF(arcSz+2*w/3,arcSz),
                                 QPointF(arcSz+2*w/3,sz-arcSz)};
        painter.drawPolygon(rightAngle,3);
    }

    // Highlights ?

    painter.end();

    return image;
}



QImage *overlayWindow::imageOfPageSelect(void)
{
    // refresh image of current page
    if( currentPageNumber >= ((int)(pagePreview.size() )) )
    {
        // Not been cached yet. Add it.
        pagePreview.push_back( annotationsImage->scaledToWidth(previewWidth) );
    }
    else
    {
        // Update current entry
        pagePreview[currentPageNumber] = annotationsImage->scaledToWidth(previewWidth);
    }

    int     entryHeight        = pagePreview[0].height()+4;
    int     numThumbNailSpaces = 1 + ((int)(pagePreview.size())) + 1; // 1 for 2*arrows, 1 for "new"

    entryWidth         = previewWidth + 8;

    qDebug() << QString("Entry width: ") << entryWidth << QString(" entryHeight: ") <<
                entryHeight << QString(" numThumbNailSpaces: ") << numThumbNailSpaces;

    pageSelectLeftArrow  = firstShownThumbNail > 0;
    pageSelectRightArrow = entryWidth*(numThumbNailSpaces-firstShownThumbNail) > width;

    QImage *image = new QImage(entryWidth*(1 + ((int)(pagePreview.size())) + 1),entryHeight,
                               QImage::Format_ARGB32_Premultiplied);

    image->fill(Qt::transparent);

    QPainter paint(image);
    paint.setRenderHint(QPainter::Antialiasing);

    paint.setBrush(Qt::white);
    paint.setPen(Qt::black);

    // Draw arrows (these have fixed screen locations)
    if( pageSelectLeftArrow )
    {
        QPointF leftAngle[3] = { QPointF(0,entryHeight/2),
                                 QPointF(entryWidth-8-entryWidth/2,8),
                                 QPointF(entryWidth-8-entryWidth/2,entryHeight-8)};
        paint.drawPolygon(leftAngle,3);
    }

    if( pageSelectRightArrow )
    {
        QPointF rightAngle[3] = { QPointF(image->width()-8,entryHeight/2),
                                  QPointF(image->width()-entryWidth/2,8),
                                  QPointF(image->width()-entryWidth/2,entryHeight-8)};
        paint.drawPolygon(rightAngle,3);
    }

    // Draw selectable pages

    int index;

    QColor bg(Qt::black);
    bg.setAlpha(33);
    paint.setBrush(bg);

    for(int t = 1; t<(numThumbNailSpaces-1); t++ )
    {
        index = t+firstShownThumbNail-1;

        qDebug() << "t " << t << " index " << index << " Size: " << pagePreview.size();

        // Draw box around image
        paint.setPen(Qt::black);
        paint.drawRect(t*entryWidth   - entryWidth/2,0,   entryWidth-1,entryHeight-1);
        paint.setPen(Qt::white);
        paint.drawRect(t*entryWidth+1 - entryWidth/2,1, entryWidth-3,entryHeight-3);

        // And stick the thumbnail in the middle
        paint.drawImage(t*entryWidth+2 - entryWidth/2,2, pagePreview[index]);
    }

    // Draw "new page"
    // Draw box around image
    paint.setPen(Qt::black);
    paint.drawRect((numThumbNailSpaces-1)*entryWidth   - entryWidth/2,0,   entryWidth-1,entryHeight-1);
    paint.setPen(Qt::white);
    paint.drawRect((numThumbNailSpaces-1)*entryWidth+1 - entryWidth/2,1, entryWidth-3,entryHeight-3);

    // And stick the text in the middle
    QString msg = tr("New Page");
    QFontMetrics metric(font());
    QSize        size = metric.size(Qt::TextSingleLine, msg);
    int          xCentre = (numThumbNailSpaces-1)*entryWidth - size.width()/2;

    paint.setPen(Qt::black);
    paint.drawText(QPointF(xCentre+2,entryHeight/2+2),msg);
    paint.setPen(Qt::white);
    paint.drawText(QPointF(xCentre,entryHeight/2),msg);

    return image;
}
