#include <QPainter>  // Won't display "start" menu when deployed to device
#include <QSvgRenderer>
#include <QIcon>
#include <QDebug>

#include "threeButtonUI.h"
#include "overlaywindow.h"

#if 1
#define METHOD_TRACE(x) qDebug() << x
#else
#define METHOD_TRACE(x)
#endif


threeButtonUI::threeButtonUI(int pseudoPenNum, bool highRes, QWidget *parent)
{
    METHOD_TRACE("threeButonUI constructor");

    // Store state data
    parentWidget   = parent;
    penNum         = pseudoPenNum;
    highResDisplay = highRes;

    if( highRes ) {
        buttonWidth   = 96;
        buttonHeight  = 96;
        menuBuffer    = 20;
    } else {
        buttonWidth   = 48;
        buttonHeight  = 48;
        menuBuffer    = 10;
    }

    keyboardAvailable = false;

    lineThickness     = 2;
    redSliderEntry    = 255;
    greenSliderEntry  = 0;
    blueSliderEntry   = 255;
    rowsDisplayed     = 3;

    // Small display positioning
    threeButtonY = buttonHeight / 4;
    menuAreaY    = threeButtonY + buttonHeight + buttonHeight / 4;

    // Don't create colour pickers unless we need them. They may use too much ram.
    colourSelector  = NULL;
    thicknessSel    = NULL;
    sliderColourSel = NULL;

    // And we are transparent

    // Create the initial widgets (they will not all be show at once, of course)

    createModeSwitchButtonBar();
    createNotificationArea();
    createPrivateButtonArea();
    createAnnotationButtonArea();
    createPresentorButtonArea();
    createMouseButtonArea();
    createThreeButtonArea();

    // Set the initial view state

    // Initial session state
    remoteEditMode                    = false;
    uiLockedUntilServerActionComplete = false;

    modeButtonsState = mode_buttons_in;

    setViewMode();
    currentTabSwitch(currentTab);

    // Hide as we will be shown by our parent
    hide();
}


void threeButtonUI::createModeSwitchButtonBar(void)
{
    METHOD_TRACE("createModeSwitchButtonBar");

    modeButtonsBar                 = new QWidget(parentWidget);
    QHBoxLayout *modeButtonsLayout = new QHBoxLayout();

    modeButtonsLayout->setMargin(0);

    // The main three (with arrows as buttons too)
    quitModeButton         = new QPushButton(parentWidget);
    viewModeButton         = new QPushButton(parentWidget);
    privateModeButton      = new QPushButton(parentWidget);
    annotateModeButton     = new QPushButton(parentWidget);
//    whiteboardModeButton   = new QPushButton(parentWidget);
//    stickyModeButton       = new QPushButton(parentWidget);
    presentationModeButton = new QPushButton(parentWidget);
    mouseModeButton        = new QPushButton(parentWidget);

    newIconResourceString = icons.view_mode;

    setIconFromResource(quitModeButton,       icons.exit_app,buttonWidth,buttonHeight);
    setIconFromResource(viewModeButton,        icons.view_mode,buttonWidth,buttonHeight);
    setIconFromResource(privateModeButton,     icons.personal_notes, buttonWidth, buttonHeight);
    setIconFromResource(annotateModeButton,    icons.annotate_mode,buttonWidth,buttonHeight);
//    setIconFromResource(whiteboardModeButton,  ":/page/images/page/whiteboard.svg",buttonWidth,buttonHeight);
//    setIconFromResource(stickyModeButton,      ":/images/stickies.svg",buttonWidth,buttonHeight);
    setIconFromResource(presentationModeButton,icons.presentation,buttonWidth,buttonHeight);
    setIconFromResource(mouseModeButton,       icons.mouse_mode,buttonWidth,buttonHeight);

    quitModeButton->setFlat(true);
    viewModeButton->setFlat(true);
    privateModeButton->setFlat(true);
    annotateModeButton->setFlat(true);
//    whiteboardModeButton->setFlat(true);
//    stickyModeButton->setFlat(true);
    presentationModeButton->setFlat(true);
    mouseModeButton->setFlat(true);

    connect(quitModeButton,         SIGNAL(clicked(bool)), this,SLOT(quitClicked(bool)) );
    connect(viewModeButton,         SIGNAL(clicked(bool)), this,SLOT(viewModeClicked(bool)) );
    connect(privateModeButton,      SIGNAL(clicked(bool)), this, SLOT(privateModeClicked(bool)) );
    connect(annotateModeButton,     SIGNAL(clicked(bool)), this,SLOT(annotateModeClicked(bool)) );
//    connect(whiteboardModeButton,   SIGNAL(clicked(bool)), this,SLOT(whiteboardModeClicked(bool)) );
//    connect(stickyModeButton,       SIGNAL(clicked(bool)), this,SLOT(stickyModeClicked(bool)) );
    connect(presentationModeButton, SIGNAL(clicked(bool)), this,SLOT(presentationModeClicked(bool)) );
    connect(mouseModeButton,        SIGNAL(clicked(bool)), this,SLOT(mouseModeClicked(bool)) );

    modeButtonsLayout->addWidget(quitModeButton,Qt::AlignRight);
    modeButtonsLayout->addWidget(viewModeButton,Qt::AlignRight);
    modeButtonsLayout->addWidget(privateModeButton,Qt::AlignRight);
    modeButtonsLayout->addWidget(annotateModeButton,Qt::AlignRight);
//    modeButtonsLayout->addWidget(whiteboardModeButton,Qt::AlignRight);
//    modeButtonsLayout->addWidget(stickyModeButton,Qt::AlignRight);
    modeButtonsLayout->addWidget(presentationModeButton,Qt::AlignRight);
    modeButtonsLayout->addWidget(mouseModeButton,Qt::AlignRight);

    modeButtonsLayout->setMargin(0);
    modeButtonsLayout->setSizeConstraint(QLayout::SetMinimumSize);

    modeButtonsBar->setLayout(modeButtonsLayout);

    modeButtonsBar->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    modeButtonsBar->setAutoFillBackground(true);
    modeButtonsBar->adjustSize();

    // Initial session state
    remoteEditMode                    = false;
    uiLockedUntilServerActionComplete = false;
    currentSessionType                = 0;
    busyMode                          = false;

    // Mainly for developmnent purposes
    modeButtonsBar->setCursor(Qt::OpenHandCursor);

    modeButtonsState = mode_buttons_in;
}

void threeButtonUI::quitClicked(bool checked)
{
    qDebug() << "Exit Attempt";
    METHOD_TRACE("quitClicked");

    overlayWindow *mw = (overlayWindow *)parentWidget;
    mw->mainMenuQuit();

    modeButtonsState = mode_buttons_move_in;
    buttonModeAnimator();
}

void threeButtonUI::viewModeClicked(bool checked)
{
    METHOD_TRACE("viewModeClicked remoteEditMode"       << remoteEditMode
                 << "uiLockedUntilServerActionComplete" << uiLockedUntilServerActionComplete);

    if( currentMode == viewMode) return;
    if( uiLockedUntilServerActionComplete ) return;

    overlayWindow *mw = (overlayWindow *)parentWidget;
    mw->disconnectFromRemoteEdit();
    mw->setZoomPanModeOn(true);

    remoteEditMode = false;

    modeButtonsState = mode_buttons_move_in;
    buttonModeAnimator();

    sessionStateChangeAnimate();

    currentTabSwitch(tabs_hidden);
    mouseButtonsBar->hide();
    presenterButtonsBar->hide();
    expandModesButton->show();
    expandMenuButton->hide();
    expandPrivateMenuButton->hide();
    menuButtonsArea->hide();
    privateMenuButtonsArea->hide();

    currentMode = viewMode;
    setExpandModeButtonsIconCurrentImage();
    qDebug() << "Entered View Mode: " << currentMode;
}

void threeButtonUI::privateModeClicked(bool checked)                                                         //pn
{
    if( uiLockedUntilServerActionComplete ) return;

    overlayWindow *mw = (overlayWindow *)parentWidget;

    displayMessage(tr("Local Annotation Enabled"), 2000);

    mw->setPenMode(penNum, APP_CTRL_PEN_MODE_LOCAL);
    mw->sessionTypePrivate(penNum);
    mw->disconnectFromRemoteEdit();
    mw->setZoomPanModeOn(false);

    modeButtonsState = mode_buttons_move_in;
    buttonModeAnimator();

    currentTabSwitch(tabs_hidden);
    mouseButtonsBar->hide();
    presenterButtonsBar->hide();
    menuButtonsArea->hide();
    privateMenuButtonsArea->hide();

    expandMenuButton->hide();
    expandPrivateMenuButton->show();
    expandModesButton->show();
    //expandMenuButton->show();

    currentMode = privateMode;
    setExpandModeButtonsIconCurrentImage();
    qDebug() << "Entered Private Mode: " << currentMode;
}

void threeButtonUI::annotateModeClicked(bool checked)
{
    displayMessage(tr("Attempting to connect"),2000);
    METHOD_TRACE("annotateModeClicked uiLockedUntilServerActionComplete"
                 << uiLockedUntilServerActionComplete);

    if( uiLockedUntilServerActionComplete ) return;

    overlayWindow *mw = (overlayWindow *)parentWidget;

    if( ! remoteEditMode )
    {
        setBusyMode();

        uiLockedUntilServerActionComplete = true;

        if( ! mw->connectToRemoteForEdit() ) {
            displayMessage(tr("Failed to get annotation session"),2000);
            uiLockedUntilServerActionComplete = false;
            return;
        }

        uiLockedUntilServerActionComplete = false;
        remoteEditMode                    = true;
    }

    displayMessage(tr("Annotation session"),2000);

    mw->setPenMode(penNum,APP_CTRL_PEN_MODE_OVERLAY);
    mw->sessionTypeOverlay(penNum);
    mw->setZoomPanModeOn(false);

    modeButtonsState = mode_buttons_move_in;
    buttonModeAnimator();

    currentTabSwitch(tabs_hidden);
    mouseButtonsBar->hide();
    presenterButtonsBar->hide();
    menuButtonsArea->hide();
    privateMenuButtonsArea->hide();

    expandMenuButton->show();
    expandPrivateMenuButton->hide();
    expandModesButton->show();
    expandMenuButton->show();
    expandPrivateMenuButton->hide();

    currentMode = overlayMode;
    setExpandModeButtonsIconCurrentImage();
    qDebug() << "Entered Overlay Mode: " << currentMode;
}

void threeButtonUI::whiteboardModeClicked(bool checked)
{
    METHOD_TRACE("whiteboardModeClicked uiLockedUntilServerActionComplete"
                 << uiLockedUntilServerActionComplete);

    if( uiLockedUntilServerActionComplete ) return;

    overlayWindow *mw = (overlayWindow *)parentWidget;

    if( ! remoteEditMode )
    {
        setBusyMode();

        uiLockedUntilServerActionComplete = true;

        if( ! mw->connectToRemoteForEdit() ) {
            displayMessage(tr("Failed to get whiteboard session"),2000);
            uiLockedUntilServerActionComplete = false;
            return;
        }

        uiLockedUntilServerActionComplete = false;
        remoteEditMode                    = true;
    }

    displayMessage(tr("Joined whiteboard session"),2000);

    mw->setZoomPanModeOn(false);
    mw->setPenMode(penNum,APP_CTRL_PEN_MODE_OVERLAY);
    mw->sessionTypeWhiteboard(penNum);
    mw->setZoomPanModeOn(false);

    modeButtonsState = mode_buttons_move_in;
    buttonModeAnimator();

    currentTabSwitch(tabs_hidden);
    mouseButtonsBar->hide();
    presenterButtonsBar->hide();
    expandModesButton->show();
    expandMenuButton->show();
    expandPrivateMenuButton->hide();
    menuButtonsArea->hide();
    privateMenuButtonsArea->hide();

    currentMode = whiteboardMode;
    setExpandModeButtonsIconCurrentImage();
     qDebug() << "Entered Whiteboard Mode " << currentMode;
}

void threeButtonUI::stickyModeClicked(bool checked)
{
    METHOD_TRACE("stickyModeClicked uiLockedUntilServerActionComplete"
                 << uiLockedUntilServerActionComplete);

    if( uiLockedUntilServerActionComplete ) return;

    overlayWindow *mw = (overlayWindow *)parentWidget;

    if( ! remoteEditMode )
    {
        setBusyMode();

        uiLockedUntilServerActionComplete = true;

        if( ! mw->connectToRemoteForEdit() ) {
            displayMessage(tr("Failed to get sticky mode session"),2000);
            uiLockedUntilServerActionComplete = false;
            return;
        }

        uiLockedUntilServerActionComplete = false;
        remoteEditMode                    = true;
    }

    displayMessage(tr("Joined sticky note session"),2000);

    mw->setZoomPanModeOn(false);
    mw->setPenMode(penNum,APP_CTRL_PEN_MODE_OVERLAY);
    mw->sessionTypeStickyNotes(penNum);

    currentTabSwitch(tabs_hidden);
    mouseButtonsBar->hide();
    presenterButtonsBar->hide();
    expandModesButton->show();
    expandMenuButton->show();
    expandPrivateMenuButton->hide();
    menuButtonsArea->hide();
    privateMenuButtonsArea->hide();

    currentMode = overlayMode;
    setExpandModeButtonsIconCurrentImage();
}

void threeButtonUI::presentationModeClicked(bool checked)
{
    METHOD_TRACE("presentationModeClickeduiLockedUntilServerActionComplete"
                 << uiLockedUntilServerActionComplete);

    if( uiLockedUntilServerActionComplete ) return;

    overlayWindow *mw = (overlayWindow *)parentWidget;

    if( ! remoteEditMode )
    {
        setBusyMode();

        uiLockedUntilServerActionComplete = true;

        if( ! mw->connectToRemoteForEdit() ) {
            displayMessage(tr("Failed to get presentation controller session"),2000);
            uiLockedUntilServerActionComplete = false;
            return;
        }

        uiLockedUntilServerActionComplete = false;
        remoteEditMode                    = true;
    }

    displayMessage(tr("Presentation controller"),2000);

    mw->setPenMode(penNum,APP_CTRL_PEN_MODE_PRESNTATION);
    mw->setZoomPanModeOn(false);

    modeButtonsState = mode_buttons_move_in;
    buttonModeAnimator();

    currentTabSwitch(tabs_hidden);
    mouseButtonsBar->hide();
    presenterButtonsBar->show();
    expandModesButton->show();
    expandMenuButton->hide();
    expandPrivateMenuButton->hide();
    menuButtonsArea->hide();
    privateMenuButtonsArea->hide();

    currentMode = presenterMode;
    setExpandModeButtonsIconCurrentImage();
    qDebug() << "Entered Presenter Mode: " << currentMode;
}

void threeButtonUI::mouseModeClicked(bool checked)
{
    METHOD_TRACE("mouseModeClicked uiLockedUntilServerActionComplete"
                 << uiLockedUntilServerActionComplete);

    if( uiLockedUntilServerActionComplete ) return;

    overlayWindow *mw = (overlayWindow *)parentWidget;

    if( ! remoteEditMode )
    {
        setBusyMode();

        uiLockedUntilServerActionComplete = true;
        remoteEditMode                    = true;
        if( ! mw->connectToRemoteForEdit() ) {
            displayMessage(tr("Failed to get mouse and keyboard session"),2000);
            uiLockedUntilServerActionComplete = false;
            return;
        }
        uiLockedUntilServerActionComplete = false;
        remoteEditMode                = false;
    }

    displayMessage(tr("Mouse controller"),2000);

    mw->setPenMode(penNum,APP_CTRL_PEN_MODE_MOUSE);
    mw->setZoomPanModeOn(false);

    modeButtonsState = mode_buttons_move_in;
    buttonModeAnimator();

    currentTabSwitch(tabs_hidden);
    mouseButtonsBar->show();
    presenterButtonsBar->hide();
    expandModesButton->show();
    expandMenuButton->hide();
    expandPrivateMenuButton->hide();
    menuButtonsArea->hide();
    privateMenuButtonsArea->hide();

    currentMode = mouseMode;
    setExpandModeButtonsIconCurrentImage();
    qDebug() << "Entered Mouse Mode: " << currentMode;
}

void threeButtonUI::createNotificationArea(void)
{
    qDebug() << "Creating Notification Area";
    METHOD_TRACE("createNotificationArea");

    notificationArea = new QWidget(parentWidget);
    QVBoxLayout *notificationLayout = new QVBoxLayout();

    notificationText = new QLabel(parentWidget);
    notificationText->setWordWrap(true);
    notificationText->setAlignment(Qt::AlignCenter);
    notificationLayout->addWidget(notificationText);

    notificationArea->setAutoFillBackground(true);
    notificationArea->setLayout(notificationLayout);
    notificationArea->setGeometry(0,0, 5*buttonWidth, 2*buttonHeight);

    hideNotificationTimer = new QTimer(this);
}

void threeButtonUI::hideNotification()
{
    notificationArea->hide();
}

void threeButtonUI::displayMessage(QString message, int durationMS)
{
    if( notificationArea->isVisible() )
    {
        hideNotificationTimer->stop();
    }

    notificationText->setText(message);
    notificationArea->show();
    notificationArea->setGeometry(threeButtonBar->x()-3*buttonWidth,threeButtonBar->y()+2*buttonHeight,
                                  5*buttonWidth, 2*buttonHeight);
    hideNotificationTimer->singleShot(durationMS,this,SLOT(hideNotification()));
}

//     Might be simpler to just use annotation function for pn but just in case
void threeButtonUI::createPrivateButtonArea(void)                                                           //pn
{
    qDebug() << "Creating Private Mode button Area";
    createPrivateUndoTabsMenu();
    createPrivateMainControlsTabsMenu();
    createPrivatePageControlsTabsMenu();
    createPrivateCaptureTabsMenu();

    privateMenuButtonsArea  = new QWidget(parentWidget);
    QVBoxLayout *pMenuArea  = new QVBoxLayout();

    pMenuArea->setMargin(0);

    QHBoxLayout *pTextTabs = createPrivateTopTabButtonArea();

    pMenuArea->addLayout(pTextTabs);
    pMenuArea->addWidget(privateUndoTabButtonsArea);
    pMenuArea->addWidget(privateMainControlsTabButtonsArea);
    pMenuArea->addWidget(privatePageControlsTabButtonsArea);
    pMenuArea->addWidget(privateCaptureTabButtonsArea);

    pMenuArea->setSizeConstraint(QLayout::SetMinimumSize);

    privateMenuButtonsArea->setLayout(pMenuArea);
    privateMenuButtonsArea->setAutoFillBackground(true);

    privateMenuButtonsArea->adjustSize();

    privateMenuButtonsArea->setCursor(Qt::OpenHandCursor);
}//*/


void threeButtonUI::createAnnotationButtonArea(void)
{
    qDebug() << "Create Annotation Button Area";

//    createStickyTabsMenu();
    createUndoTabsMenu();
    createMainControlsTabsMenu();
//    createPenShapesTabsMenu();
    createPageControlsTabsMenu();
    createCaptureTabsMenu();

    menuButtonsArea       = new QWidget(parentWidget);
    QVBoxLayout *menuArea = new QVBoxLayout();

    menuArea->setMargin(0);

    QHBoxLayout *textTabs = createTopTabButtonArea();

    menuArea->addLayout(textTabs);
//    menuArea->addWidget(stickyTabButtonsArea);
    menuArea->addWidget(undoTabButtonsArea);
    menuArea->addWidget(mainControlsTabButtonsArea);
//    menuArea->addWidget(penShapesTabButtonsArea);
    menuArea->addWidget(pageControlsTabButtonsArea);
    menuArea->addWidget(captureTabButtonsArea);

    /*QHBoxLayout *printStickyText = printTextHeaders("Sticky Notes");
    stickyNotesArea              = createStickyButtonArea();
    QHBoxLayout *printUndoText   = printTextHeaders("Undo/Redo");
    QHBoxLayout *undoRedo        = createMainMenuUndoRedoRow();
    QHBoxLayout *mainControls    = createMainMenuControlsRow();
    QHBoxLayout *penShape        = createMainMenuPenShapeRow();
    QHBoxLayout *pageControls    = createMainMenuPageControlsRow();
    QHBoxLayout *captureControls = createMainMenuCaptureControlsRow();

    menuArea->addLayout(printStickyText);
    menuArea->addLayout(stickyNotesArea);
    menuArea->addLayout(printUndoText);
    menuArea->addLayout(undoRedo);
    menuArea->addLayout(mainControls); // pen colour, thickness, paper colour
    menuArea->addLayout(penShape);
    menuArea->addLayout(pageControls); // Foreward/backwards/select/clear...
    menuArea->addLayout(captureControls);*/
    menuArea->setSizeConstraint(QLayout::SetMinimumSize);

    menuButtonsArea->setLayout(menuArea);
    menuButtonsArea->setAutoFillBackground(true);

    menuButtonsArea->adjustSize();

    // Mainly for developmnent purposes
    menuButtonsArea->setCursor(Qt::OpenHandCursor);
}
#if 0
void threeButtonUI::createStickyTabsMenu(void)
{
    stickyTabButtonsArea       = new QWidget(parentWidget);
    QVBoxLayout *stickyTabArea = new QVBoxLayout();

    stickyTabArea->setMargin(0);

//    QHBoxLayout *topTabArea             = createTopTabButtonArea();
//    QHBoxLayout *printStickiesText      = printTextHeaders("Stickies");
    stickyNotesArea                     = createStickyButtonArea();

//    stickyTabArea->addLayout(topTabArea);
//    stickyTabArea->addLayout(printStickiesText);
    stickyTabArea->addLayout(stickyNotesArea);
    stickyTabArea->setSizeConstraint(QLayout::SetMinimumSize);

    stickyTabButtonsArea->setLayout(stickyTabArea);
    stickyTabButtonsArea->setAutoFillBackground(true);

    stickyTabButtonsArea->adjustSize();

    // Mainly for developmnent purposes
    stickyTabButtonsArea->setCursor(Qt::OpenHandCursor);
}
#endif
//
void threeButtonUI::createPrivateUndoTabsMenu(void)                                              //pn
{
    qDebug() << "Create Private Undo Tab Menu";
    privateUndoTabButtonsArea = new QWidget(parentWidget);
    QVBoxLayout *pUndoTabArea = new QVBoxLayout();

    QHBoxLayout *pUndoRedo    = createPrivateMainMenuUndoRedoRow();

    pUndoTabArea->addLayout(pUndoRedo);
    pUndoTabArea->setSizeConstraint(QLayout::SetMinimumSize);

    privateUndoTabButtonsArea->setLayout(pUndoTabArea);
    privateUndoTabButtonsArea->setAutoFillBackground(true);

    privateUndoTabButtonsArea->adjustSize();

    privateUndoTabButtonsArea->setCursor(Qt::OpenHandCursor);
}//*/

void threeButtonUI::createUndoTabsMenu(void)
{
    qDebug() << "Create Undo Tab Menu";
    undoTabButtonsArea       = new QWidget(parentWidget);
    QVBoxLayout *undoTabArea = new QVBoxLayout();

    undoTabArea->setMargin(0);

//    QHBoxLayout *topTabArea      = createTopTabButtonArea();
//    QHBoxLayout *printUndoText   = printTextHeaders("Undo");
    QHBoxLayout *undoRedo        = createMainMenuUndoRedoRow();
    QHBoxLayout *textRow1        = undoRedoText();

//    undoTabArea->addLayout(topTabArea);
//    undoTabArea->addLayout(printUndoText);
    undoTabArea->addLayout(undoRedo);
    undoTabArea->addLayout(textRow1);
    undoTabArea->setSizeConstraint(QLayout::SetMinimumSize);

    undoTabButtonsArea->setLayout(undoTabArea);
    undoTabButtonsArea->setAutoFillBackground(true);

    undoTabButtonsArea->adjustSize();

    // Mainly for developmnent purposes
    undoTabButtonsArea->setCursor(Qt::OpenHandCursor);
}
//
void threeButtonUI::createPrivateMainControlsTabsMenu(void)                                                  //pn
{
    qDebug() << "Create Private Main Control Tab Menu";
    privateMainControlsTabButtonsArea         = new QWidget(parentWidget);
    QVBoxLayout *privateMainControlsTabArea   = new QVBoxLayout();

    privateMainControlsTabArea->setMargin(0);

    QVBoxLayout *pMainControls           = createPrivateMainMenuControlsRow();

    privateMainControlsTabArea->addLayout(pMainControls);
    privateMainControlsTabArea->setSizeConstraint(QLayout::SetMinimumSize);

    privateMainControlsTabButtonsArea->setLayout(privateMainControlsTabArea);
    privateMainControlsTabButtonsArea->setAutoFillBackground(true);

    privateMainControlsTabButtonsArea->adjustSize();

    // Mainly for developmnent purposes
    privateMainControlsTabButtonsArea->setCursor(Qt::OpenHandCursor);
}//*/

void threeButtonUI::createMainControlsTabsMenu(void)
{
    qDebug() << "Create Main Control Tab Menu";
    mainControlsTabButtonsArea         = new QWidget(parentWidget);
    QVBoxLayout *mainControlsTabArea   = new QVBoxLayout();

    mainControlsTabArea->setMargin(0);

//    QHBoxLayout *topTabArea             = createTopTabButtonArea();
//    QHBoxLayout *printMainControlsText  = printTextHeaders("Main Controls");
    QVBoxLayout *mainControls           = createMainMenuControlsRow();

//    mainControlsTabArea->addLayout(topTabArea);
//    mainControlsTabArea->addLayout(printMainControlsText);
    mainControlsTabArea->addLayout(mainControls);
    mainControlsTabArea->setSizeConstraint(QLayout::SetMinimumSize);

    mainControlsTabButtonsArea->setLayout(mainControlsTabArea);
    mainControlsTabButtonsArea->setAutoFillBackground(true);

    mainControlsTabButtonsArea->adjustSize();

    // Mainly for developmnent purposes
    mainControlsTabButtonsArea->setCursor(Qt::OpenHandCursor);
}

/*
void threeButtonUI::createPenShapesTabsMenu(void)
{
    penShapesTabButtonsArea         = new QWidget(parentWidget);
    QVBoxLayout *penShapesTabArea   = new QVBoxLayout();

    penShapesTabArea->setMargin(0);

//    QHBoxLayout *topTabArea             = createTopTabButtonArea();
//    QHBoxLayout *printMainControlsText  = printTextHeaders("Pen Shapes");
    QHBoxLayout *penShape               = createMainMenuPenShapeRow();

//    penShapesTabArea->addLayout(topTabArea);
//    penShapesTabArea->addLayout(printMainControlsText);
    penShapesTabArea->addLayout(penShape);
    penShapesTabArea->setSizeConstraint(QLayout::SetMinimumSize);

    penShapesTabButtonsArea->setLayout(penShapesTabArea);
    penShapesTabButtonsArea->setAutoFillBackground(true);

    penShapesTabButtonsArea->adjustSize();

    // Mainly for developmnent purposes
    penShapesTabButtonsArea->setCursor(Qt::OpenHandCursor);
}
*/
//
void threeButtonUI::createPrivatePageControlsTabsMenu(void)                                                  //pn
{
    qDebug() << "Create Private Page Controls Tab Menu";
    privatePageControlsTabButtonsArea       = new QWidget(parentWidget);
    QVBoxLayout *privatePageControlsTabArea = new QVBoxLayout();

    privatePageControlsTabArea->setMargin(0);

    QVBoxLayout *privatePageControls        = createPrivateMainMenuPageControlsRow(); //Original Line
    //QVBoxLayout *privatePageControls        = createPrivatePageControlsTabsMenu();

    privatePageControlsTabArea->addLayout(privatePageControls);
    privatePageControlsTabArea->setSizeConstraint(QLayout::SetMinimumSize);

    privatePageControlsTabButtonsArea->setLayout(privatePageControlsTabArea);
    privatePageControlsTabButtonsArea->setAutoFillBackground(true);

    privatePageControlsTabButtonsArea->adjustSize();

    privatePageControlsTabButtonsArea->setCursor(Qt::OpenHandCursor);
}//*/

void threeButtonUI::createPageControlsTabsMenu(void)
{
    qDebug() << "Create Page Controls Tab Menu";
    pageControlsTabButtonsArea         = new QWidget(parentWidget);
    QVBoxLayout *pageControlsTabArea   = new QVBoxLayout();

    pageControlsTabArea->setMargin(0);

//    QHBoxLayout *topTabArea             = createTopTabButtonArea();
//    QHBoxLayout *printMainControlsText  = printTextHeaders("Page Controls");
    QVBoxLayout *pageControls           = createMainMenuPageControlsRow(); //Original Line
    //QVBoxLayout *pageControls           = createPageControlsTabsMenu();

//    pageControlsTabArea->addLayout(topTabArea);
//    pageControlsTabArea->addLayout(printMainControlsText);
    pageControlsTabArea->addLayout(pageControls);
    pageControlsTabArea->setSizeConstraint(QLayout::SetMinimumSize);

    pageControlsTabButtonsArea->setLayout(pageControlsTabArea);
    pageControlsTabButtonsArea->setAutoFillBackground(true);

    pageControlsTabButtonsArea->adjustSize();

    // Mainly for developmnent purposes
    pageControlsTabButtonsArea->setCursor(Qt::OpenHandCursor);
}
//
void threeButtonUI::createPrivateCaptureTabsMenu(void)                                                       //pn
{
    qDebug() << "Create Private Capture Tab Menu";
    privateCaptureTabButtonsArea         = new QWidget(parentWidget);
    QVBoxLayout *pCaptureTabArea         = new QVBoxLayout();

    pCaptureTabArea->setMargin(0);

    QHBoxLayout *privateCaptureControls = createMainMenuCaptureControlsRow();

    pCaptureTabArea->addLayout(privateCaptureControls);
    pCaptureTabArea->setSizeConstraint(QLayout::SetMinimumSize);

    privateCaptureTabButtonsArea->setLayout(pCaptureTabArea);
    privateCaptureTabButtonsArea->setAutoFillBackground(true);

    privateCaptureTabButtonsArea->adjustSize();

    // Mainly for developmnent purposes
    privateCaptureTabButtonsArea->setCursor(Qt::OpenHandCursor);
}//*/

void threeButtonUI::createCaptureTabsMenu(void)
{
    qDebug() << "Create Capture Tab Menu";
    captureTabButtonsArea         = new QWidget(parentWidget);
    QVBoxLayout *captureTabArea   = new QVBoxLayout();

    captureTabArea->setMargin(0);

//    QHBoxLayout *topTabArea             = createTopTabButtonArea();
//    QHBoxLayout *printMainControlsText  = printTextHeaders("Capture");
    QHBoxLayout *captureControls        = createMainMenuCaptureControlsRow();
    QHBoxLayout *captureControlsText    = saveSnapshotText();

//    captureTabArea->addLayout(topTabArea);
//    captureTabArea->addLayout(printMainControlsText);
    captureTabArea->addLayout(captureControls);
    captureTabArea->addLayout(captureControlsText);
    captureTabArea->setSizeConstraint(QLayout::SetMinimumSize);

    captureTabButtonsArea->setLayout(captureTabArea);
    captureTabButtonsArea->setAutoFillBackground(true);

    captureTabButtonsArea->adjustSize();

    // Mainly for developmnent purposes
    captureTabButtonsArea->setCursor(Qt::OpenHandCursor);
}
//
QHBoxLayout *threeButtonUI::createPrivateTopTabButtonArea(void)                                                  //pn
{
    qDebug() << "Create Private Top Tab Button Area";
    QHBoxLayout *pRow = new QHBoxLayout();

    privatePenTabs       = new QPushButton(tr("pPen"),  parentWidget);
    privateUndoTabs      = new QPushButton(tr("pUndo"), parentWidget);
    privateClearPageTabs = new QPushButton(tr("pPage"), parentWidget);
    privateSnapshotTabs  = new QPushButton(tr("pSave"), parentWidget);

    privatePenTabs->setFlat(true);
    privateUndoTabs->setFlat(true);
    privateClearPageTabs->setFlat(true);
    privateSnapshotTabs->setFlat(true);

    connect(privatePenTabs,       SIGNAL(clicked(bool)), this, SLOT(privateLinesClicked(bool))    );
    connect(privateUndoTabs,      SIGNAL(clicked(bool)), this, SLOT(privateUndoClicked(bool))     );
    connect(privateClearPageTabs, SIGNAL(clicked(bool)), this, SLOT(privatePageClicked(bool))     );
    connect(privateSnapshotTabs,  SIGNAL(clicked(bool)), this, SLOT(privateSnapshotClicked(bool)) );

    pRow->addWidget(privatePenTabs,       Qt::AlignLeft);
    pRow->addWidget(privateUndoTabs,      Qt::AlignLeft);
    pRow->addWidget(privateClearPageTabs, Qt::AlignLeft);
    pRow->addWidget(privateSnapshotTabs,  Qt::AlignLeft);

    pRow->setSizeConstraint(QLayout::SetMinimumSize);

    return pRow;
}//*/

QHBoxLayout *threeButtonUI::createTopTabButtonArea(void)
{
    qDebug() << "Create Top Tab Button Area";
    QHBoxLayout *row = new QHBoxLayout();

    penTabs        = new QPushButton(tr("Pen"),parentWidget);
    undoTabs       = new QPushButton(tr("Undo"),parentWidget);
//    QPushButton *penColourTabs  = new QPushButton(tr("Colour"),parentWidget);
    clearPageTabs  = new QPushButton(tr("Page"),parentWidget);
//    newStickyTabs  = new QPushButton(tr("Sticky"),parentWidget);
    snapshotTabs   = new QPushButton(tr("Save"),parentWidget);

//    setIconFromResource(newStickyTabs,  ":/stickies/images/stickies/new.svg",       buttonWidth,buttonHeight);
//    setIconFromResource(undoTabs,       ":/images/top/undo.svg",                    buttonWidth,buttonHeight);
//    setIconFromResource(penColourTabs,  ":/pen/images/pen/colour.svg",              buttonWidth,buttonHeight);
//    setIconFromResource(lineTabs,       ":/shapes/images/shapes/line.svg",          buttonWidth,buttonHeight);
//    setIconFromResource(clearPageTabs,  ":/page/images/page/clear.svg",             buttonWidth,buttonHeight);
//    setIconFromResource(snapshotTabs,   ":/capture/images/capture/snapshot.svg",    buttonWidth,buttonHeight);

//    newStickyTabs->setFlat(true);
    undoTabs->setFlat(true);
//    penColourTabs->setFlat(true);
    penTabs->setFlat(true);
    clearPageTabs->setFlat(true);
    snapshotTabs->setFlat(true);

//    connect(newStickyTabs,  SIGNAL(clicked(bool)),  this,SLOT(stickyClicked(bool)) );
    connect(undoTabs,       SIGNAL(clicked(bool)),  this,SLOT(undoClicked(bool)) );
//    connect(penColourTabs,  SIGNAL(clicked(bool)),  this,SLOT(colourClicked(bool)) );
    connect(penTabs,       SIGNAL(clicked(bool)),  this,SLOT(linesClicked(bool)) );
    connect(clearPageTabs,  SIGNAL(clicked(bool)),  this,SLOT(pageClicked(bool)) );
    connect(snapshotTabs,   SIGNAL(clicked(bool)),  this,SLOT(snapshotClicked(bool)) );

    row->addWidget(penTabs,Qt::AlignLeft);
//    row->addWidget(penColourTabs,Qt::AlignLeft);
    row->addWidget(undoTabs,Qt::AlignLeft);
    row->addWidget(clearPageTabs,Qt::AlignLeft);
//    row->addWidget(newStickyTabs,Qt::AlignLeft);
    row->addWidget(snapshotTabs,Qt::AlignLeft);

    row->setSizeConstraint(QLayout::SetMinimumSize);

    return row;
}

void threeButtonUI::stickyClicked(bool checked)
{
    currentTabSwitch(tab_sticky);
}
//
void threeButtonUI::privateUndoClicked(bool checked)                                                    //pn
{
    currentTabSwitch(tab_p_undo);
}//*/

void threeButtonUI::undoClicked(bool checked)
{
    currentTabSwitch(tab_undo);
}

//void threeButtonUI::colourClicked(bool checked)
//{
//    currentTabSwitch(tab_main_controls);
//}
//
void threeButtonUI::privateLinesClicked(bool checked)                                                   //pn
{
    currentTabSwitch(tab_p_main_controls);
}//*/

void threeButtonUI::linesClicked(bool checked)
{
    currentTabSwitch(tab_main_controls);
}
//
void threeButtonUI::privatePageClicked(bool checked)                                                     //pn
{
    currentTabSwitch(tab_p_page_controls);
}//*/

void threeButtonUI::pageClicked(bool checked)
{
    currentTabSwitch(tab_page_controls);
}
//
void threeButtonUI::privateSnapshotClicked(bool checked)                                                 //pn
{
    currentTabSwitch(tab_p_capture);
}//*/

void threeButtonUI::snapshotClicked(bool checked)
{
    currentTabSwitch(tab_capture);
}

void threeButtonUI::currentTabSwitch(overlay_tab_t changedTab)
{
    qDebug() << "Trying to change Tabs " << currentTab;
    currentTab = changedTab;
    switch(currentTab)
    {
        case tabs_hidden:
//            stickyTabButtonsArea->hide();
            privateUndoTabButtonsArea->hide();
            undoTabButtonsArea->hide();
            privateMainControlsTabButtonsArea->hide();
            mainControlsTabButtonsArea->hide();
//            penShapesTabButtonsArea->hide();
            privatePageControlsTabButtonsArea->hide();
            pageControlsTabButtonsArea->hide();
            privateCaptureTabButtonsArea->hide();
            captureTabButtonsArea->hide();
            rowsDisplayed = 1;
        break;
        case tab_sticky:
//            stickyTabButtonsArea->show();
            privateUndoTabButtonsArea->hide();
            undoTabButtonsArea->hide();
            privateMainControlsTabButtonsArea->hide();
            mainControlsTabButtonsArea->hide();
//            penShapesTabButtonsArea->hide();
            privatePageControlsTabButtonsArea->hide();
            pageControlsTabButtonsArea->hide();
            privateCaptureTabButtonsArea->hide();
            captureTabButtonsArea->hide();
            rowsDisplayed = 1;
        break;
        case tab_undo:
//            stickyTabButtonsArea->hide();
            privateUndoTabButtonsArea->hide();
            undoTabButtonsArea->show();
            privateMainControlsTabButtonsArea->hide();
            mainControlsTabButtonsArea->hide();
//            penShapesTabButtonsArea->hide();
            privatePageControlsTabButtonsArea->hide();
            pageControlsTabButtonsArea->hide();
            privateCaptureTabButtonsArea->hide();
            captureTabButtonsArea->hide();
            rowsDisplayed = 1;
        break;
        case tab_main_controls:
//            stickyTabButtonsArea->hide();
            privateUndoTabButtonsArea->hide();
            undoTabButtonsArea->hide();
            privateMainControlsTabButtonsArea->hide();
            mainControlsTabButtonsArea->show();
//            penShapesTabButtonsArea->hide();
            privatePageControlsTabButtonsArea->hide();
            pageControlsTabButtonsArea->hide();
            privateCaptureTabButtonsArea->hide();
            captureTabButtonsArea->hide();
            rowsDisplayed = 3;
        break;
//        case tab_shapes:
//            stickyTabButtonsArea->hide();
//            undoTabButtonsArea->hide();
//            mainControlsTabButtonsArea->hide();
//            penShapesTabButtonsArea->show();
//            pageControlsTabButtonsArea->hide();
//            captureTabButtonsArea->hide();
//            rowsDisplayed = 1;
//        break;
        case tab_page_controls:
//            stickyTabButtonsArea->hide();
            privateUndoTabButtonsArea->hide();
            undoTabButtonsArea->hide();
            privateMainControlsTabButtonsArea->hide();
            mainControlsTabButtonsArea->hide();
//            penShapesTabButtonsArea->hide();
            privatePageControlsTabButtonsArea->hide();
            pageControlsTabButtonsArea->show();
            privateCaptureTabButtonsArea->hide();
            captureTabButtonsArea->hide();
            rowsDisplayed = 2;
        break;
        case tab_capture:
//            stickyTabButtonsArea->hide();
            privateUndoTabButtonsArea->hide();
            undoTabButtonsArea->hide();
            privateMainControlsTabButtonsArea->hide();
            mainControlsTabButtonsArea->hide();
//            penShapesTabButtonsArea->hide();
            privatePageControlsTabButtonsArea->hide();
            pageControlsTabButtonsArea->hide();
            privateCaptureTabButtonsArea->hide();
            captureTabButtonsArea->show();
            rowsDisplayed = 1;
        break;
        case tab_p_undo:
            privateUndoTabButtonsArea->show();
            undoTabButtonsArea->hide();
            privateMainControlsTabButtonsArea->hide();
            mainControlsTabButtonsArea->hide();
            privatePageControlsTabButtonsArea->hide();
            pageControlsTabButtonsArea->hide();
            privateCaptureTabButtonsArea->hide();
            captureTabButtonsArea->hide();
            rowsDisplayed = 1;
        break;
        case tab_p_main_controls:
            privateUndoTabButtonsArea->hide();
            undoTabButtonsArea->hide();
            privateMainControlsTabButtonsArea->show();
            mainControlsTabButtonsArea->hide();
            privatePageControlsTabButtonsArea->hide();
            pageControlsTabButtonsArea->hide();
            privateCaptureTabButtonsArea->hide();
            captureTabButtonsArea->hide();
            rowsDisplayed = 3;
        break;
        case tab_p_page_controls:
            privateUndoTabButtonsArea->hide();
            undoTabButtonsArea->hide();
            privateMainControlsTabButtonsArea->hide();
            mainControlsTabButtonsArea->hide();
            privatePageControlsTabButtonsArea->show();
            pageControlsTabButtonsArea->hide();
            privateCaptureTabButtonsArea->hide();
            captureTabButtonsArea->hide();
            rowsDisplayed = 2;
        break;
        case tab_p_capture:
            privateUndoTabButtonsArea->hide();
            undoTabButtonsArea->hide();
            privateMainControlsTabButtonsArea->hide();
            mainControlsTabButtonsArea->hide();
            privatePageControlsTabButtonsArea->hide();
            pageControlsTabButtonsArea->hide();
            privateCaptureTabButtonsArea->show();
            captureTabButtonsArea->hide();
            rowsDisplayed = 1;
        break;//*/
    }
    qDebug() << "Hopefully changed tabs " << currentTab;
    screenResized(parentWidth, parentHeight);   // Adjust menu area size for number of rows
//    mainControlsTabButtonsArea->repaint();
}

QHBoxLayout *threeButtonUI::createStickyButtonArea(void)
{
    qDebug() << "Create Sticky Button Area";
    QHBoxLayout *row = new QHBoxLayout();

    QPushButton *newSticky     = new QPushButton(parentWidget);
    QPushButton *moveSticky    = new QPushButton(parentWidget);
    QPushButton *stickyColour  = new QPushButton(parentWidget);
    QPushButton *snapSticky    = new QPushButton(parentWidget);
    QPushButton *arrangeSticky = new QPushButton(parentWidget);
    QPushButton *deleteSticky  = new QPushButton(parentWidget);

    setIconFromResource(newSticky,    ":/stickies/images/stickies/new.svg",    buttonWidth,buttonHeight);
    setIconFromResource(moveSticky,   ":/stickies/images/stickies/move.svg",   buttonWidth,buttonHeight);
    setIconFromResource(stickyColour, ":/menus/images/paper_colour.svg",       buttonWidth,buttonHeight);
    setIconFromResource(snapSticky,   ":/stickies/images/stickies/snap.svg",   buttonWidth,buttonHeight);
    setIconFromResource(arrangeSticky,":/stickies/images/stickies/arrange.svg",buttonWidth,buttonHeight);
    setIconFromResource(deleteSticky, ":/stickies/images/stickies/delete.svg", buttonWidth,buttonHeight);

    newSticky->setFlat(true);
    moveSticky->setFlat(true);
    stickyColour->setFlat(true);
    snapSticky->setFlat(true);
    arrangeSticky->setFlat(true);
    deleteSticky->setFlat(true);

    connect(newSticky,     SIGNAL(clicked(bool)), this,SLOT(addNewStickyStart(bool)) );
    connect(moveSticky,    SIGNAL(clicked(bool)), this,SLOT(startmoveofStickyNote(bool)) );
    connect(stickyColour,  SIGNAL(clicked(bool)), this,SLOT(setStickyNotePaperColour(bool)) );
    connect(snapSticky,    SIGNAL(clicked(bool)), this,SLOT(dummyMenuModeCall(bool)) );
    connect(arrangeSticky, SIGNAL(clicked(bool)), this,SLOT(dummyMenuModeCall(bool)) );
    connect(deleteSticky,  SIGNAL(clicked(bool)), this,SLOT(deleteStickyMode(bool)) );

    row->addWidget(newSticky,Qt::AlignLeft);
    row->addWidget(moveSticky,Qt::AlignLeft);
    row->addWidget(stickyColour,Qt::AlignLeft);
    row->addWidget(snapSticky,Qt::AlignLeft);
    row->addWidget(arrangeSticky,Qt::AlignLeft);
    row->addWidget(deleteSticky,Qt::AlignLeft);

    row->setSizeConstraint(QLayout::SetMinimumSize);

    return row;
}
//
QHBoxLayout *threeButtonUI::createPrivateMainMenuUndoRedoRow(void)                                               //pn
{
    qDebug() << "Create Private Main Menu Undo Redo Row";
    QHBoxLayout *pRow     = new QHBoxLayout(parentWidget);

    QPushButton *pUndo    = new QPushButton(parentWidget);
    QPushButton *pRedo    = new QPushButton(parentWidget);
    QPushButton *pDoAgain = new QPushButton(parentWidget);

    setIconFromResource(pUndo,    icons.undo,  buttonWidth,buttonHeight);
    setIconFromResource(pRedo,    icons.redo,  buttonWidth,buttonHeight);
    //setIconFromResource(pDoAgain, ":/images/do_again.svg",  buttonWidth,buttonHeight);

    pUndo->setFlat(true);
    pRedo->setFlat(true);
    //pDoAgain->setFlat(true);

    connect(pUndo,    SIGNAL(clicked(bool)), this,SLOT(undoPrivatePenAction(bool)));
    connect(pRedo,    SIGNAL(clicked(bool)), this,SLOT(redoPrivatePenAction(bool)));
    //connect(pDoAgain, SIGNAL(clicked(bool)), this,SLOT(privateBrushFromLast(bool)));

    pRow->addWidget(pUndo,    Qt::AlignLeft);
    pRow->addWidget(pRedo,    Qt::AlignLeft);
    //pRow->addWidget(pDoAgain, Qt::AlignLeft);

    pRow->setSizeConstraint(QLayout::SetMinimumSize);

    return pRow;
}//*/

QHBoxLayout *threeButtonUI::createMainMenuUndoRedoRow(void)
{
    qDebug() << "Create Main Menu Undo Redo Row";
    QHBoxLayout *row = new QHBoxLayout();

    QPushButton *undo    = new QPushButton(parentWidget);
    QPushButton *redo    = new QPushButton(parentWidget);
    QPushButton *doAgain = new QPushButton(parentWidget);

    setIconFromResource(undo,    icons.undo,     buttonWidth,buttonHeight);
    setIconFromResource(redo,    icons.redo,     buttonWidth,buttonHeight);
    //setIconFromResource(doAgain, ":/images/do_again.svg",     buttonWidth,buttonHeight);

    undo->setFlat(true);
    redo->setFlat(true);
    //doAgain->setFlat(true);

    connect(undo,     SIGNAL(clicked(bool)), this,SLOT(undoPenAction(bool)) );
    connect(redo,     SIGNAL(clicked(bool)), this,SLOT(redoPenAction(bool)) );
    //connect(doAgain,  SIGNAL(clicked(bool)), this,SLOT(brushFromLast(bool)) );

    row->addWidget(undo,Qt::AlignLeft);
    row->addWidget(redo,Qt::AlignLeft);
    //row->addWidget(doAgain,Qt::AlignLeft);

    row->setSizeConstraint(QLayout::SetMinimumSize);

    return row;
}
//
QVBoxLayout *threeButtonUI::createPrivateMainMenuControlsRow(void)                                                      //pn
{
    qDebug() << "Create Private Main Menu Controls Row";
    QVBoxLayout *pCol  = new QVBoxLayout();
    QHBoxLayout *pRow1 = new QHBoxLayout();
    QHBoxLayout *pRow2 = new QHBoxLayout();
    QHBoxLayout *textRow1 = new QHBoxLayout();
    QHBoxLayout *textRow2 = new QHBoxLayout();

    QPushButton *pPenColour     = new QPushButton(parentWidget);
    QPushButton *pPenThickness  = new QPushButton(parentWidget);

    //QPushButton *personalPen      = new QPushButton(parentWidget);
    //QPushButton *personalHLighter = new QPushButton(parentWidget);
    QPushButton *personalText     = new QPushButton(parentWidget);
    //QPushButton *personalEraser   = new QPushButton(parentWidget);

    setIconFromResource(pPenColour,       icons.colour_palette,      buttonWidth,buttonHeight);
    setIconFromResource(pPenThickness,    icons.thickness,   buttonWidth,buttonHeight);

    //setIconFromResource(personalPen,      ":/nib/images/nib/pen.svg",         buttonWidth,buttonHeight);
    //setIconFromResource(personalHLighter, ":/nib/images/nib/highlighter.svg", buttonWidth,buttonHeight);
    setIconFromResource(personalText,     icons.text_entry,        buttonWidth,buttonHeight);
    //setIconFromResource(personalEraser,   ":/nib/images/nib/eraser.svg",      buttonWidth,buttonHeight);

    pPenColour->setFlat(true);
    pPenThickness->setFlat(true);

    //personalPen->setFlat(true);
    //personalHLighter->setFlat(true);
    personalText->setFlat(true);
    //personalEraser->setFlat(true);

    connect(pPenColour,       SIGNAL(clicked(bool)), this,SLOT(chooseNewPenColour(bool))     );
    connect(pPenThickness,    SIGNAL(clicked(bool)), this,SLOT(chooseNewPenThickness(bool))  );

    //connect(personalPen,      SIGNAL(clicked(bool)), this,SLOT(dummyMenuModeCall(bool))      );
    //connect(personalHLighter, SIGNAL(clicked(bool)), this,SLOT(dummyMenuModeCall(bool))      );
    connect(personalText,     SIGNAL(clicked(bool)), this,SLOT(togglePrivateTextInput(bool)) );
    //connect(personalEraser,   SIGNAL(clicked(bool)), this,SLOT(dummyMenuModeCall(bool))      );

    pRow1->addWidget(pPenColour,       Qt::AlignLeft);
    pRow1->addWidget(pPenThickness,    Qt::AlignLeft);

    //pRow2->addWidget(personalPen,      Qt::AlignLeft);
    //pRow2->addWidget(personalHLighter, Qt::AlignLeft);
    pRow2->addWidget(personalText,     Qt::AlignLeft);
    //pRow2->addWidget(personalEraser,   Qt::AlignLeft);

    pRow1->setSizeConstraint(QLayout::SetMinimumSize);
    pRow2->setSizeConstraint(QLayout::SetMinimumSize);

    textRow1 = penTypeText();
    textRow2 = penShapeText();

    //QHBoxLayout *pRow3 = createPrivateMainMenuPenShapeRow();

    pCol->addLayout(pRow1);
    pCol->addLayout(pRow2);
    //pCol->addLayout(pRow3);

    return pCol;
}//*/

 // pen colour, thickness, paper colour
QVBoxLayout *threeButtonUI::createMainMenuControlsRow(void)
{
    qDebug() << "Create Main Menu Controls Row";
    QVBoxLayout *col  = new QVBoxLayout();
    QHBoxLayout *row1 = new QHBoxLayout();
    QHBoxLayout *row2 = new QHBoxLayout();
    QHBoxLayout *textRow1 = new QHBoxLayout();
    QHBoxLayout *textRow2 = new QHBoxLayout();

    QPushButton *penColour       = new QPushButton(parentWidget);
    QPushButton *penThickness    = new QPushButton(parentWidget);

//    QPushButton *typePen         = new QPushButton(parentWidget);
//    QPushButton *typeHighlighter = new QPushButton(parentWidget);
    QPushButton *typeText        = new QPushButton(parentWidget);
//    QPushButton *typeEraser      = new QPushButton(parentWidget);

    setIconFromResource(penColour,    icons.colour_palette,     buttonWidth,buttonHeight);
    setIconFromResource(penThickness, icons.thickness,  buttonWidth,buttonHeight);

//    setIconFromResource(typePen,         ":/nib/images/nib/pen.svg",         buttonWidth,buttonHeight);
//    setIconFromResource(typeHighlighter, ":/nib/images/nib/highlighter.svg", buttonWidth,buttonHeight);
    setIconFromResource(typeText,        icons.text_entry,        buttonWidth,buttonHeight);
//    setIconFromResource(typeEraser,      ":/nib/images/nib/eraser.svg",      buttonWidth,buttonHeight);

    penColour->setFlat(true);
    penThickness->setFlat(true);

//    typePen->setFlat(true);
//    typeHighlighter->setFlat(true);
    typeText->setFlat(true);
//    typeEraser->setFlat(true);

    connect(penColour,    SIGNAL(clicked(bool)), this,SLOT(chooseNewPenColour(bool)) );
    connect(penThickness, SIGNAL(clicked(bool)), this,SLOT(chooseNewPenThickness(bool)) );

//    connect(typePen,         SIGNAL(clicked(bool)), this,SLOT(dummyMenuModeCall(bool)) );
//    connect(typeHighlighter, SIGNAL(clicked(bool)), this,SLOT(dummyMenuModeCall(bool)) );
    connect(typeText,        SIGNAL(clicked(bool)), this,SLOT(toggleTextInput(bool)) );
//    connect(typeEraser,      SIGNAL(clicked(bool)), this,SLOT(dummyMenuModeCall(bool)) );

    row1->addWidget(penColour,Qt::AlignLeft);
    row1->addWidget(penThickness,Qt::AlignLeft);

//    row2->addWidget(typePen,Qt::AlignLeft);
//    row2->addWidget(typeHighlighter,Qt::AlignLeft);
    row1->addWidget(typeText,Qt::AlignLeft);
//    row2->addWidget(typeEraser,Qt::AlignLeft);

    row1->setSizeConstraint(QLayout::SetMinimumSize);
    row2->setSizeConstraint(QLayout::SetMinimumSize);

    textRow1 = penTypeText();
    textRow2 = penShapeText();

    QHBoxLayout *row3 = createMainMenuPenShapeRow();

    col->addLayout(row1);
    col->addLayout(textRow1);
//    col->addLayout(row2);
    col->addLayout(row3);
    col->addLayout(textRow2);

    return col;
}
//
QHBoxLayout *threeButtonUI::createPrivateMainMenuPenShapeRow(void)                                                   //pn
{
    qDebug() << "Create Private Main Menu Pen Shape Row";
    QHBoxLayout *pRow      = new QHBoxLayout();
    QPushButton *pLine     = new QPushButton();
    QPushButton *pTriangle = new QPushButton();
    QPushButton *pBox      = new QPushButton();
    QPushButton *pCircle   = new QPushButton();
    QPushButton *pStamp    = new QPushButton();

    setIconFromResource(pLine,     icons.freehand,     buttonWidth,buttonHeight);
    setIconFromResource(pTriangle, icons.triangle_stamp, buttonWidth,buttonHeight);
    setIconFromResource(pBox,      icons.square_stamp,   buttonWidth,buttonHeight);
    setIconFromResource(pCircle,   icons.circle_stamp,   buttonWidth,buttonHeight);
    setIconFromResource(pStamp,    icons.clone_stamp,    buttonWidth,buttonHeight);

    pLine->setFlat(true);
    pTriangle->setFlat(true);
    pBox->setFlat(true);
    pCircle->setFlat(true);
    pStamp->setFlat(true);

    connect(pLine,     SIGNAL(clicked(bool)), this,SLOT(privateDrawTypeLine(bool))    );
    connect(pTriangle, SIGNAL(clicked(bool)), this,SLOT(privateDrawTypeTriangle(bool)));
    connect(pBox,      SIGNAL(clicked(bool)), this,SLOT(privateDrawTypeBox(bool))     );
    connect(pCircle,   SIGNAL(clicked(bool)), this,SLOT(privateDrawTypeCircle(bool))  );
    connect(pStamp,    SIGNAL(clicked(bool)), this,SLOT(privateDrawTypeStamp(bool))   );

    pRow->addWidget(pLine,     Qt::AlignLeft);
    pRow->addWidget(pTriangle, Qt::AlignLeft);
    pRow->addWidget(pBox,      Qt::AlignLeft);
    pRow->addWidget(pCircle,   Qt::AlignLeft);
    pRow->addWidget(pStamp,    Qt::AlignLeft);

    pRow->setSizeConstraint(QLayout::SetMinimumSize);

    return pRow;
}//*/

QHBoxLayout *threeButtonUI::createMainMenuPenShapeRow(void)
{
    qDebug() << "Create Main Menu Pen Shape Row";
    QHBoxLayout *row = new QHBoxLayout();

    QPushButton *line     = new QPushButton(parentWidget);
    QPushButton *triangle = new QPushButton(parentWidget);
    QPushButton *box      = new QPushButton(parentWidget);
    QPushButton *circle   = new QPushButton(parentWidget);
    QPushButton *stamp    = new QPushButton(parentWidget);

    setIconFromResource(line,     icons.freehand,     buttonWidth,buttonHeight);
    setIconFromResource(triangle, icons.triangle_stamp, buttonWidth,buttonHeight);
    setIconFromResource(box,      icons.square_stamp,   buttonWidth,buttonHeight);
    setIconFromResource(circle,   icons.circle_stamp,   buttonWidth,buttonHeight);
    setIconFromResource(stamp,    icons.clone_stamp,    buttonWidth,buttonHeight);

    line->setFlat(true);
    triangle->setFlat(true);
    box->setFlat(true);
    circle->setFlat(true);
    stamp->setFlat(true);

    connect(line,     SIGNAL(clicked(bool)), this,SLOT(drawTypeLine(bool)) );
    connect(triangle, SIGNAL(clicked(bool)), this,SLOT(drawTypeTriangle(bool)) );
    connect(box,      SIGNAL(clicked(bool)), this,SLOT(drawTypeBox(bool)) );
    connect(circle,   SIGNAL(clicked(bool)), this,SLOT(drawTypeCircle(bool)) );
    connect(stamp,    SIGNAL(clicked(bool)), this,SLOT(drawTypeStamp(bool)) );

    row->addWidget(line,Qt::AlignLeft);
    row->addWidget(triangle,Qt::AlignLeft);
    row->addWidget(box,Qt::AlignLeft);
    row->addWidget(circle,Qt::AlignLeft);
    row->addWidget(stamp,Qt::AlignLeft);

    row->setSizeConstraint(QLayout::SetMinimumSize);

    return row;
}
//
QVBoxLayout *threeButtonUI::createPrivateMainMenuPageControlsRow(void)                                              //pn
{
    qDebug() << "Create Private Main Menu Page Control Row";
    QVBoxLayout *pCol  = new QVBoxLayout();
    QHBoxLayout *pRow1 = new QHBoxLayout();
    QHBoxLayout *pRow2 = new QHBoxLayout();

    QPushButton *newPPage     = new QPushButton(parentWidget);
    QPushButton *nextPPage    = new QPushButton(parentWidget);
    QPushButton *lastPPage    = new QPushButton(parentWidget);
    QPushButton *selectPPage  = new QPushButton(parentWidget);
    QPushButton *clearPPage   = new QPushButton(parentWidget);
    QPushButton *pPaperColour = new QPushButton(parentWidget);
    QPushButton *clearPPaper  = new QPushButton(parentWidget);

    setIconFromResource(newPPage,     icons.new_page,       buttonWidth,buttonHeight);
    setIconFromResource(nextPPage,    icons.next_page,      buttonWidth,buttonHeight);
    setIconFromResource(lastPPage,    icons.prev_page,     buttonWidth,buttonHeight);
    setIconFromResource(selectPPage,  icons.display_thumb, buttonWidth,buttonHeight);
    setIconFromResource(clearPPage,   icons.undo_all,            buttonWidth,buttonHeight);
    setIconFromResource(clearPPaper,  icons.undo_all,     buttonWidth,buttonHeight);
    setIconFromResource(pPaperColour, icons.colour_palette,  buttonWidth,buttonHeight);

    newPPage->setFlat(true);
    nextPPage->setFlat(true);
    lastPPage->setFlat(true);
    selectPPage->setFlat(true);
    clearPPage->setFlat(true);
    clearPPaper->setFlat(true);
    pPaperColour->setFlat(true);

    connect(newPPage,     SIGNAL(clicked(bool)), this,SLOT(newPage(bool))                  );
    connect(nextPPage,    SIGNAL(clicked(bool)), this,SLOT(nextPage(bool))                 );
    connect(lastPPage,    SIGNAL(clicked(bool)), this,SLOT(prevPage(bool))                 );
    connect(selectPPage,  SIGNAL(clicked(bool)), this,SLOT(showPageSelector(bool))         );
    connect(clearPPage,   SIGNAL(clicked(bool)), this,SLOT(clearPOverlay(bool))            );
    connect(pPaperColour, SIGNAL(clicked(bool)), this,SLOT(chooseNewPPaperColour(bool))    );
    connect(clearPPaper,  SIGNAL(clicked(bool)), this,SLOT(transparentPrivateOverlay(bool)));

    pRow1->addWidget(clearPPage,    Qt::AlignLeft);
    pRow1->addWidget(pPaperColour,  Qt::AlignLeft);
    pRow1->addWidget(clearPPaper,   Qt::AlignLeft);

    pRow2->addWidget(newPPage,      Qt::AlignLeft);
    pRow2->addWidget(nextPPage,     Qt::AlignLeft);
    pRow2->addWidget(lastPPage,     Qt::AlignLeft);
    pRow2->addWidget(selectPPage,   Qt::AlignLeft);

    pRow1->setSizeConstraint(QLayout::SetMinimumSize);
    pRow2->setSizeConstraint(QLayout::SetMinimumSize);

    pCol->addLayout(pRow1);
    pCol->addLayout(pRow2);

    pCol->setSizeConstraint(QLayout::SetMinimumSize);

    return pCol;
}//*/

QVBoxLayout *threeButtonUI::createMainMenuPageControlsRow(void)
{
    qDebug() << "Create Main Menu Page Control Row";
    QVBoxLayout *col  = new QVBoxLayout();
    QHBoxLayout *row1 = new QHBoxLayout();
    QHBoxLayout *textRow1 = new QHBoxLayout();
    QHBoxLayout *row2 = new QHBoxLayout();
    QHBoxLayout *textRow2 = new QHBoxLayout();

    textRow1 = pageBackgroundText();
    textRow2 = changePageNumText();

    QPushButton *newPage     = new QPushButton(parentWidget);
    QPushButton *nextPage    = new QPushButton(parentWidget);
    QPushButton *lastPage    = new QPushButton(parentWidget);
    QPushButton *selectPage  = new QPushButton(parentWidget);
    QPushButton *clearPage   = new QPushButton(parentWidget);
    QPushButton *paperColour = new QPushButton(parentWidget);
    QPushButton *clearPaper  = new QPushButton(parentWidget);

    setIconFromResource(newPage,     icons.new_page,       buttonWidth,buttonHeight);
    setIconFromResource(nextPage,    icons.next_page,      buttonWidth,buttonHeight);
    setIconFromResource(lastPage,    icons.prev_page,     buttonWidth,buttonHeight);
    setIconFromResource(selectPage,  icons.display_thumb, buttonWidth,buttonHeight);
    setIconFromResource(clearPage,   icons.undo_all,            buttonWidth,buttonHeight);
    setIconFromResource(clearPaper,  icons.undo_all,     buttonWidth,buttonHeight);
    setIconFromResource(paperColour, icons.colour_palette,  buttonWidth,buttonHeight);

    newPage->setFlat(true);
    nextPage->setFlat(true);
    lastPage->setFlat(true);
    selectPage->setFlat(true);
    clearPage->setFlat(true);
    paperColour->setFlat(true);
    clearPaper->setFlat(true);

    connect(newPage,     SIGNAL(clicked(bool)), this,SLOT(newPage(bool)) );
    connect(nextPage,    SIGNAL(clicked(bool)), this,SLOT(nextPage(bool)) );
    connect(lastPage,    SIGNAL(clicked(bool)), this,SLOT(prevPage(bool)) );
    connect(selectPage,  SIGNAL(clicked(bool)), this,SLOT(showPageSelector(bool)) );
    connect(clearPage,   SIGNAL(clicked(bool)), this,SLOT(clearOverlay(bool)) );
    connect(paperColour, SIGNAL(clicked(bool)), this,SLOT(chooseNewPaperColour(bool)) );
    connect(clearPaper,  SIGNAL(clicked(bool)), this,SLOT(transparentOverlay(bool)) );

    row1->addWidget(clearPage,Qt::AlignLeft);
    row1->addWidget(paperColour,Qt::AlignLeft);
    row1->addWidget(clearPaper,Qt::AlignLeft);

    row2->addWidget(newPage,Qt::AlignLeft);
    row2->addWidget(nextPage,Qt::AlignLeft);
    row2->addWidget(lastPage,Qt::AlignLeft);
    row2->addWidget(selectPage,Qt::AlignLeft);

    row1->setSizeConstraint(QLayout::SetMinimumSize);
    row2->setSizeConstraint(QLayout::SetMinimumSize);

    col->addLayout(row1);
    col->addLayout(textRow1);
    col->addLayout(row2);
    col->addLayout(textRow2);

    col->setSizeConstraint(QLayout::SetMinimumSize);

    return col;
}

// Private Note version unnessissary
QHBoxLayout *threeButtonUI::createMainMenuCaptureControlsRow(void)
{
    qDebug() << "Create Main Menu Capture Control Row";
    QHBoxLayout *row = new QHBoxLayout();

    QPushButton *record   = new QPushButton(parentWidget);
    QPushButton *playback = new QPushButton(parentWidget);
    QPushButton *snapshot = new QPushButton(parentWidget);
    QPushButton *gallery  = new QPushButton(parentWidget);
    QPushButton *save     = new QPushButton(parentWidget);

    setIconFromResource(record,   icons.record,   buttonWidth,buttonHeight);
    setIconFromResource(playback, icons.play_video,     buttonWidth,buttonHeight);
    setIconFromResource(snapshot, icons.snapshot, buttonWidth,buttonHeight);
    setIconFromResource(gallery,  icons.display_thumb,  buttonWidth,buttonHeight);
    setIconFromResource(save,     ":/images/old_files/capture/save.svg",     buttonWidth,buttonHeight);

    record->setFlat(true);
    playback->setFlat(true);
    snapshot->setFlat(true);
    gallery->setFlat(true);
    save->setFlat(true);

    connect(record,   SIGNAL(clicked(bool)), this,SLOT(recordDiscussion(bool)) );
    connect(playback, SIGNAL(clicked(bool)), this,SLOT(playbackDiscussion(bool)) );
    connect(snapshot, SIGNAL(clicked(bool)), this,SLOT(localScreenSnap(bool)) );
    connect(gallery,  SIGNAL(clicked(bool)), this,SLOT(dummyMenuModeCall(bool)) );
    connect(save,     SIGNAL(clicked(bool)), this,SLOT(dummyMenuModeCall(bool)) );

    row->addWidget(record,Qt::AlignLeft);
    row->addWidget(playback,Qt::AlignLeft);
    row->addWidget(snapshot,Qt::AlignLeft);
    row->addWidget(gallery,Qt::AlignLeft);
    row->addWidget(save,Qt::AlignLeft);

    row->setSizeConstraint(QLayout::SetMinimumSize);

    return row;
}

/****************************************************************
 *
 * Below are all of the text rows for each annotation tab.
 * Beware, they are annoying.
 *
 * They are, in order:
 *
 * Pen type
 * Pen shape
 * -
 * Undo/redo
 * -
 * Page background
 * Change page number
 * -
 * Save/snapshot
 *
 *
 *
 * After that, there is Mouse and Presentation modes.
 *
 ***************************************************************/

QHBoxLayout *threeButtonUI::penTypeText(void)
{
    QHBoxLayout *row = new QHBoxLayout();
    QPushButton *penColourText = new QPushButton(tr("Colour"),parentWidget);
    QPushButton *penThicknessText = new QPushButton(tr("Thickness"),parentWidget);
    QPushButton *penTypeText = new QPushButton(tr("Send Text"),parentWidget);

    penColourText->setFlat(true);
    penThicknessText->setFlat(true);
    penTypeText->setFlat(true);
    row->addWidget(penColourText,Qt::AlignLeft);
    row->addWidget(penThicknessText,Qt::AlignLeft);
    row->addWidget(penTypeText,Qt::AlignLeft);
    return row;
}

QHBoxLayout *threeButtonUI::penShapeText(void)
{
    QHBoxLayout *row = new QHBoxLayout();
    QPushButton *lineText = new QPushButton(tr("Line"),parentWidget);
    QPushButton *triangleText = new QPushButton(tr("Tri"),parentWidget);
    QPushButton *squareText = new QPushButton(tr("Squ"),parentWidget);
    QPushButton *circleText = new QPushButton(tr("Circle"),parentWidget);
    QPushButton *stampText = new QPushButton(tr("Stamp"),parentWidget);

    lineText->setFlat(true);
    triangleText->setFlat(true);
    squareText->setFlat(true);
    circleText->setFlat(true);
    stampText->setFlat(true);

    row->addWidget(lineText,Qt::AlignLeft);
    row->addWidget(triangleText,Qt::AlignLeft);
    row->addWidget(squareText,Qt::AlignLeft);
    row->addWidget(circleText,Qt::AlignLeft);
    row->addWidget(stampText,Qt::AlignLeft);
    return row;
}

QHBoxLayout *threeButtonUI::undoRedoText(void)
{
    QHBoxLayout *row = new QHBoxLayout();
    QPushButton *undoText = new QPushButton(tr("Undo"),parentWidget);
    QPushButton *redoText = new QPushButton(tr("Redo"),parentWidget);

    undoText->setFlat(true);
    redoText->setFlat(true);

    row->addWidget(undoText,Qt::AlignLeft);
    row->addWidget(redoText,Qt::AlignLeft);

    return row;

}

QHBoxLayout *threeButtonUI::pageBackgroundText(void)
{
    QHBoxLayout *row = new QHBoxLayout();
    QPushButton *clearPageText = new QPushButton(tr("Clear Lines"),parentWidget);
    QPushButton *bgColorText = new QPushButton(tr("Fill BG"),parentWidget);
    QPushButton *clearBGText = new QPushButton(tr("Clear BG"),parentWidget);

    clearPageText->setFlat(true);
    bgColorText->setFlat(true);
    clearBGText->setFlat(true);

    row->addWidget(clearPageText,Qt::AlignLeft);
    row->addWidget(bgColorText,Qt::AlignLeft);
    row->addWidget(clearBGText,Qt::AlignLeft);

    return row;

}

QHBoxLayout *threeButtonUI::changePageNumText(void)
{
    QHBoxLayout *row = new QHBoxLayout();
    QPushButton *newPageText = new QPushButton(tr("New Page"),parentWidget);
    QPushButton *nextPageText = new QPushButton(tr("Next"),parentWidget);
    QPushButton *lastPageText = new QPushButton(tr("Last"),parentWidget);
    QPushButton *selectPageText = new QPushButton(tr("Gallery"),parentWidget);

    newPageText->setFlat(true);
    nextPageText->setFlat(true);
    lastPageText->setFlat(true);
    selectPageText->setFlat(true);

    row->addWidget(newPageText,Qt::AlignLeft);
    row->addWidget(nextPageText,Qt::AlignLeft);
    row->addWidget(lastPageText,Qt::AlignLeft);
    row->addWidget(selectPageText,Qt::AlignLeft);

    return row;

}

QHBoxLayout *threeButtonUI::saveSnapshotText(void)
{
    QHBoxLayout *row = new QHBoxLayout();
    QPushButton *recordText = new QPushButton(tr("Record"),parentWidget);
    QPushButton *playbackText = new QPushButton(tr("Playback"),parentWidget);
    QPushButton *snapshotText = new QPushButton(tr("Snapshot"),parentWidget);

    recordText->setFlat(true);
    playbackText->setFlat(true);
    snapshotText->setFlat(true);

    row->addWidget(recordText,Qt::AlignLeft);
    row->addWidget(playbackText,Qt::AlignLeft);
    row->addWidget(snapshotText,Qt::AlignLeft);

    return row;
}

QHBoxLayout *threeButtonUI::printTextHeaders(QString displayText)
{
    QHBoxLayout *row = new QHBoxLayout();
    QLabel *testText = new QLabel;
    QFont font;
    font.setPointSize(14);
    testText->setFont(font);
    testText->setText(displayText); //new QLabel(tr(displayText));
    row->addWidget(testText);
    row->setSizeConstraint(QLayout::SetMinimumSize);

    return row;
}

void threeButtonUI::dummyMenuModeCall(bool checked)
{
    displayMessage("Not written yet. Features comming soon.",4000);
    //menuButtonsArea->hide();
    currentTabSwitch(tab_main_controls);
}

#if 0
// Reference calls
void threeButtonUI::dummy(void)
{
    overlayWindow *mw = (overlayWindow *)parentWidget;

    mw->penThicknessChanged(penNum,thickness);
    mw->enableTextInputWindow(keyboardAvailable);
    mw->setPenShape(penNum, (shape_t)((long)(menuEntry->data)) );
    mw->remoteNetSelectAndConnect(penNum);
    mw->allowRemoteNetSession(penNum);
    mw->toggleMouseAsPen();
    mw->sessionTypeOverlay(penNum);
    mw->sessionTypeWhiteboard(penNum);
    mw->sessionTypeStickyNotes(penNum);
}
#endif
//Change penNum here when testing. *Expected* Will work *Tested* -Not Yet.
void threeButtonUI::privateDrawTypeLine(bool checked)                                                            //pn
{
    overlayWindow *mw = (overlayWindow *)parentWidget;
    mw->setPenShape(penNum, SHAPE_NONE);
    privateMenuButtonsArea->hide();
    currentTabSwitch(tabs_hidden);
}

void threeButtonUI::privateDrawTypeTriangle(bool checked)                                                       //pn
{
    overlayWindow *mw = (overlayWindow *)parentWidget;
    mw->setPenShape(penNum, SHAPE_TRIANGLE);
    privateMenuButtonsArea->hide();
    currentTabSwitch(tabs_hidden);
}

void threeButtonUI::privateDrawTypeBox(bool checked)                                                            //pn
{
    overlayWindow *mw = (overlayWindow *)parentWidget;
    mw->setPenShape(penNum, SHAPE_BOX);
    privateMenuButtonsArea->hide();
    currentTabSwitch(tabs_hidden);
}

void threeButtonUI::privateDrawTypeCircle(bool checked)                                                          //pn
{
    overlayWindow *mw = (overlayWindow *)parentWidget;
    mw->setPenShape(penNum, SHAPE_CIRCLE);
    privateMenuButtonsArea->hide();
    currentTabSwitch(tabs_hidden);
}

void threeButtonUI::privateDrawTypeStamp(bool checked)                                                           //pn
{
    overlayWindow *mw = (overlayWindow *)parentWidget;
    mw->brushFromLast(penNum);
    privateMenuButtonsArea->hide();
    currentTabSwitch(tabs_hidden);
}//*/

void threeButtonUI::drawTypeLine(bool checked)
{
    overlayWindow *mw = (overlayWindow *)parentWidget;
    mw->setPenShape(penNum, SHAPE_NONE );
    menuButtonsArea->hide();
    currentTabSwitch(tabs_hidden);
}

void threeButtonUI::drawTypeTriangle(bool checked)
{
    overlayWindow *mw = (overlayWindow *)parentWidget;
    mw->setPenShape(penNum, SHAPE_TRIANGLE );
    menuButtonsArea->hide();
    currentTabSwitch(tabs_hidden);
}

void threeButtonUI::drawTypeBox(bool checked)
{
    overlayWindow *mw = (overlayWindow *)parentWidget;
    mw->setPenShape(penNum, SHAPE_BOX );
    menuButtonsArea->hide();
    currentTabSwitch(tabs_hidden);
}

void threeButtonUI::drawTypeCircle(bool checked)
{
    overlayWindow *mw = (overlayWindow *)parentWidget;
    mw->setPenShape(penNum, SHAPE_CIRCLE );
    menuButtonsArea->hide();
    currentTabSwitch(tabs_hidden);
}

void threeButtonUI::drawTypeStamp(bool checked)
{
    overlayWindow *mw = (overlayWindow *)parentWidget;
    mw->brushFromLast(penNum);
    menuButtonsArea->hide();
    currentTabSwitch(tabs_hidden);
}

void threeButtonUI::localScreenSnap(bool checked)
{
    overlayWindow *mw = (overlayWindow *)parentWidget;
    mw->localScreenSnap();
    menuButtonsArea->hide();
    currentTabSwitch(tabs_hidden);
}
void threeButtonUI::remoteScreenSnap(bool checked)
{
    overlayWindow *mw = (overlayWindow *)parentWidget;
    mw->screenSnap();
    menuButtonsArea->hide();
    currentTabSwitch(tabs_hidden);
}

void threeButtonUI::getColourSelector(void)
{
    if( colourSelector == NULL )
    {
        colourSelector = new QColorDialog(parentWidget);

        colourSelector->setWindowFlags(Qt::Dialog);

        // Both native & non native are way too big... hopefully qt will fix this soon.
        // colourSelector->setOptions(QColorDialog::DontUseNativeDialog);

        connect(colourSelector,SIGNAL(colorSelected(QColor &colour)),this,SLOT(colourSelected(QColor &newColour)));
        connect(colourSelector,SIGNAL(accepted()),this,SLOT(colourAccepted()));
        connect(colourSelector,SIGNAL(rejected()),this,SLOT(colourSelectCancelled()));
    }
}

// public
//     void     paperColourChanged(int penNum, QColor newColour);
//     void     penColourChanged(int penNum, QColor newColour);
//     void     penThicknessChanged(int penNum, int newThickness);
// private slots:
//
void     threeButtonUI::chooseNewPenColour(bool checked)
{
    if( parentHeight < 550 ) {
        chooseNewSliderColour(checked);
        return;
    }
    overlayWindow *mw = (overlayWindow *)parentWidget;

    // block buttons until colour selection is done
    uiLockedUntilServerActionComplete = true;

    // Launch a colour picker dialogue
    colourSelectMode = pen_colour;
    getColourSelector();

    colourSelector->setCurrentColor(mw->getLocalPenColour());

    colourSelector->setWindowTitle(tr("Select new pen colour"));
    colourSelector->show();

    menuButtonsArea->hide();
    privateMenuButtonsArea->hide();
    currentTabSwitch(tabs_hidden);
}
void     threeButtonUI::colourAccepted()
{
    QColor newCol = colourSelector->selectedColor();
    colourSelected(newCol);
}
void     threeButtonUI::colourSelected(QColor &newColour)
{
    overlayWindow *mw = (overlayWindow *)parentWidget;
    switch( colourSelectMode ) {
    case pen_colour:          mw->penColourChanged(penNum,newColour);    break;
    case whiteboard_colour:   mw->paperColourChanged(penNum,newColour);  break;
    case priv_board_colour:   mw->privPaperColourChanged(newColour);     break;
    case sticky_note_colour:  mw->paperColourChanged(penNum,newColour);  break;
    }

    uiLockedUntilServerActionComplete = false;
    colourSelector->hide();
}
void     threeButtonUI::colourSelectCancelled()
{
    uiLockedUntilServerActionComplete = false;
    colourSelector->hide();
}

void threeButtonUI::chooseNewSliderColour(bool checked)
{
#ifdef TABLET_MODE
    if( sliderColourSel == NULL )
    {
        sliderColourSel = new colourSlider(parentWidget);

        connect(sliderColourSel, SIGNAL(newSliderColourSelected(int,int,int)), this, SLOT(newSliderValue(int,int,int)));
        connect(sliderColourSel, SIGNAL(cancelSliderColourChange()), this, SLOT(cancelSliderColourSel()));
    }

    sliderColourSel->setSliderColour(redSliderEntry, greenSliderEntry, blueSliderEntry);
    sliderColourSel->show();

    menuButtonsArea->hide();
    privateMenuButtonsArea->hide();
    currentTabSwitch(tabs_hidden);
#endif
}

void threeButtonUI::newSliderValue(int redSliderValue, int greenSliderValue, int blueSliderValue)
{
    QColor combiColourValue;

    sliderColourSel->hide();
    redSliderEntry   = redSliderValue;
    greenSliderEntry = greenSliderValue;
    blueSliderEntry  = blueSliderValue;

    combiColourValue = QColor( (redSliderValue), (greenSliderValue), (blueSliderValue) );
    overlayWindow *mw = (overlayWindow *)parentWidget;
    mw->penColourChanged(penNum, combiColourValue);
}

void threeButtonUI::cancelSliderColourSel()
{
    sliderColourSel->hide();
}

void     threeButtonUI::chooseNewPenThickness(bool checked)
{
#ifdef TABLET_MODE
    // Launch a slider select dialogue
    if( thicknessSel == NULL )
    {
        thicknessSel = new lineThicknessSelector(parentWidget);

        connect(thicknessSel,SIGNAL(newThicknessSelected(int)),this,SLOT(newThickness(int)));
        connect(thicknessSel,SIGNAL(cancelThicknessChange()),  this,SLOT(cancelledThiocknesSel()));
    }

    thicknessSel->setThickness(lineThickness);
    thicknessSel->show();

    menuButtonsArea->hide();
    privateMenuButtonsArea->hide();
    currentTabSwitch(tabs_hidden);
#endif
}

void     threeButtonUI::newThickness(int newThickness)
{
#ifdef TABLET_MODE
    thicknessSel->hide();

    lineThickness = newThickness;

    overlayWindow *mw = (overlayWindow *)parentWidget;
    mw->penThicknessChanged(penNum,newThickness);
#endif
}

void     threeButtonUI::cancelledThiocknesSel()
{
#ifdef TABLET_MODE
    thicknessSel->hide();
#endif
}

void threeButtonUI::chooseNewPPaperColour(bool checked)                                                               //pn
{
    overlayWindow *mw = (overlayWindow *)parentWidget;

    uiLockedUntilServerActionComplete = true;

    colourSelectMode = priv_board_colour;
    getColourSelector();

    colourSelector->setCurrentColor(mw->getLocalPenColour());

    colourSelector->setWindowTitle(tr("Select Background Colour"));
    colourSelector->show();

    privateMenuButtonsArea->hide();
    currentTabSwitch(tabs_hidden);
}//*/

void     threeButtonUI::chooseNewPaperColour(bool checked)
{
    overlayWindow *mw = (overlayWindow *)parentWidget;

    // block buttons until colour selection is done
    uiLockedUntilServerActionComplete = true;

    // Launch a colour picker dialogue
    colourSelectMode = whiteboard_colour;
    getColourSelector();

    colourSelector->setCurrentColor(mw->getLocalPenColour());

    colourSelector->setWindowTitle(tr("Select Background Colour"));
    colourSelector->show();

    menuButtonsArea->hide();
    currentTabSwitch(tabs_hidden);
}
//*
void threeButtonUI::transparentPrivateOverlay(bool checked)                                                            //pn
{
    overlayWindow *mw = (overlayWindow *)parentWidget;
    mw->privPaperColourChanged(Qt::transparent);;

    privateMenuButtonsArea->hide();
    currentTabSwitch(tabs_hidden);
}//*/

void     threeButtonUI::transparentOverlay(bool checked)
{
    // Set the page colour to transparent & hide the buttons
    overlayWindow *mw = (overlayWindow *)parentWidget;
    mw->paperColourTransparent(penNum);
//    mw->sessionTypeOverlay(penNum);
//    mw->setPenMode(penNum,APP_CTRL_PEN_MODE_OVERLAY);

    menuButtonsArea->hide();
    currentTabSwitch(tabs_hidden);
}
//
void threeButtonUI::togglePrivateTextInput(bool checked)                                                                //pn
{
    overlayWindow *mw = (overlayWindow *)parentWidget;
    keyboardAvailable = ! keyboardAvailable;
    mw->enableTextInputWindow(keyboardAvailable);
    if( keyboardAvailable )
        mw->setPenType(penNum, DRAW_TEXT);
    else
        mw->setPenType(penNum, DRAW_SOLID);
    privateMenuButtonsArea->hide();
    currentTabSwitch(tabs_hidden);
}//*/

void     threeButtonUI::toggleTextInput(bool checked)
{
    overlayWindow *mw = (overlayWindow *)parentWidget;
    keyboardAvailable = ! keyboardAvailable;
    mw->enableTextInputWindow(keyboardAvailable);
    if( keyboardAvailable )
        mw->setPenType(penNum, DRAW_TEXT);
    else
        mw->setPenType(penNum, DRAW_SOLID);
    menuButtonsArea->hide();
    currentTabSwitch(tabs_hidden);
}
//
void threeButtonUI::undoPrivatePenAction(bool checked)
{
    overlayWindow *mw = (overlayWindow *)parentWidget;
    mw->undoPrivateLines();
}

void threeButtonUI::redoPrivatePenAction(bool checked)
{
    overlayWindow *mw = (overlayWindow *)parentWidget;
    mw->redoPenAction(penNum);
}

void     threeButtonUI::undoPenAction(bool checked)
{
    overlayWindow *mw = (overlayWindow *)parentWidget;
    mw->undoPenAction(penNum);
}
void     threeButtonUI::redoPenAction(bool checked)
{
    overlayWindow *mw = (overlayWindow *)parentWidget;
    mw->redoPenAction(penNum);
}
void     threeButtonUI::brushFromLast(bool checked)
{
    overlayWindow *mw = (overlayWindow *)parentWidget;
    mw->brushFromLast(penNum);
    menuButtonsArea->hide();
    privateMenuButtonsArea->hide();
    currentTabSwitch(tabs_hidden);
}
void     threeButtonUI::newPage(bool checked)
{
    overlayWindow *mw = (overlayWindow *)parentWidget;
    mw->newPage();
}
void     threeButtonUI::nextPage(bool checked)
{
    overlayWindow *mw = (overlayWindow *)parentWidget;
    mw->localScreenSnap();
    mw->nextPage();
}
void     threeButtonUI::prevPage(bool checked)
{
    overlayWindow *mw = (overlayWindow *)parentWidget;
    mw->prevPage();
}
void     threeButtonUI::showPageSelector(bool checked)
{
    overlayWindow *mw = (overlayWindow *)parentWidget;
    mw->showPageSelector();
    menuButtonsArea->hide();
    currentTabSwitch(tabs_hidden);
}

void threeButtonUI::clearPOverlay(bool checked)
{
    overlayWindow *mw = (overlayWindow *)parentWidget;
    mw->clearPrivateNotes();
    //privateNoteDrawTrace *pn = (privateNoteDrawTrace *) parentWidget;
    //pn->resetPrivateDrawCache();
    privateMenuButtonsArea->hide();
    currentTabSwitch(tabs_hidden);
}

void     threeButtonUI::clearOverlay(bool checked)
{
    qDebug() << "Really? Really?!";
    overlayWindow *mw = (overlayWindow *)parentWidget;
    mw->clearOverlay();
    menuButtonsArea->hide();
    currentTabSwitch(tabs_hidden);
}

///////////////////////////////ADD PRIVATE STICKY STUFF HERE IF NEEDED//////////////////////////

void     threeButtonUI::addNewStickyStart(bool checked)
{
    overlayWindow *mw = (overlayWindow *)parentWidget;
    mw->addNewStickyStart(penNum);
    menuButtonsArea->hide();
    currentTabSwitch(tabs_hidden);
}
void     threeButtonUI::deleteStickyMode(bool checked)
{
    overlayWindow *mw = (overlayWindow *)parentWidget;
    mw->deleteStickyMode(penNum);
    menuButtonsArea->hide();
    currentTabSwitch(tabs_hidden);
}
void     threeButtonUI::startmoveofStickyNote(bool checked)
{
    overlayWindow *mw = (overlayWindow *)parentWidget;
    mw->startmoveofStickyNote(penNum);
    menuButtonsArea->hide();
    currentTabSwitch(tabs_hidden);
}
void     threeButtonUI::setStickyNotePaperColour(bool checked)
{
    overlayWindow *mw = (overlayWindow *)parentWidget;

    // block buttons until colour selection is done
    uiLockedUntilServerActionComplete = true;

    // Launch a colour picker dialogue
    colourSelectMode = sticky_note_colour;
    getColourSelector();

    colourSelector->setCurrentColor(mw->getLocalPenColour());

    colourSelector->setWindowTitle(tr("Select new sticky note colour"));
    colourSelector->show();

    menuButtonsArea->hide();
    currentTabSwitch(tabs_hidden);
}

void     threeButtonUI::hostedModeToggle(bool checked)
{
    overlayWindow *mw = (overlayWindow *)parentWidget;
    mw->hostedModeToggle(penNum);
    menuButtonsArea->hide();
    currentTabSwitch(tabs_hidden);
}
void     threeButtonUI::recordDiscussion(bool checked)
{
    overlayWindow *mw = (overlayWindow *)parentWidget;
    mw->recordDiscussion();
    menuButtonsArea->hide();
    currentTabSwitch(tabs_hidden);
}
void     threeButtonUI::playbackDiscussion(bool checked)
{
    overlayWindow *mw = (overlayWindow *)parentWidget;
    mw->playbackDiscussion();
    menuButtonsArea->hide();
    currentTabSwitch(tabs_hidden);
}


void threeButtonUI::createPresentorButtonArea(void)
{
    presenterButtonsBar                = new QWidget(parentWidget);
    QHBoxLayout *presnterButtonsLayout = new QHBoxLayout();

    presnterButtonsLayout->setMargin(0);

    // The main three (with arrows as buttons too)
    prevSlideButton           = new QPushButton(parentWidget);
    nextSlideButton           = new QPushButton(parentWidget);
    toggleLaserButton         = new QPushButton(parentWidget);
    startPresentationButton   = new QPushButton(parentWidget);
    stopPresentationButton    = new QPushButton(parentWidget);
    defaultPresentationButton = new QPushButton(parentWidget);

    setIconFromResource(prevSlideButton,           icons.prev_slide,buttonWidth,buttonHeight);
    setIconFromResource(nextSlideButton,           icons.next_slide,buttonWidth,buttonHeight);
    setIconFromResource(toggleLaserButton,         icons.laser_pointer,buttonWidth,buttonHeight);
    setIconFromResource(startPresentationButton,   icons.start_presentation,buttonWidth,buttonHeight);
    setIconFromResource(stopPresentationButton,    icons.stop_presentation,buttonWidth,buttonHeight);
    setIconFromResource(defaultPresentationButton, icons.start_presentation2,buttonWidth,buttonHeight);

    prevSlideButton->setFlat(true);
    nextSlideButton->setFlat(true);
    toggleLaserButton->setFlat(true);
    startPresentationButton->setFlat(true);
    stopPresentationButton->setFlat(true);
    defaultPresentationButton->setFlat(true);

    connect(prevSlideButton,           SIGNAL(clicked(bool)), this,SLOT(prevSlideClicked(bool)) );
    connect(nextSlideButton,           SIGNAL(clicked(bool)), this,SLOT(nextSlideClicked(bool)) );
    connect(toggleLaserButton,         SIGNAL(clicked(bool)), this,SLOT(toggleLaserClicked(bool)) );
    connect(startPresentationButton,   SIGNAL(clicked(bool)), this,SLOT(startPresentationClicked(bool)) );
    connect(stopPresentationButton,    SIGNAL(clicked(bool)), this,SLOT(stopPresentationClicked(bool)) );
    connect(defaultPresentationButton, SIGNAL(clicked(bool)), this,SLOT(defaultPresentationClicked(bool)) );

    presnterButtonsLayout->addWidget(prevSlideButton,Qt::AlignRight);
    presnterButtonsLayout->addWidget(nextSlideButton,Qt::AlignRight);
    presnterButtonsLayout->addWidget(toggleLaserButton,Qt::AlignRight);
    presnterButtonsLayout->addWidget(startPresentationButton,Qt::AlignRight);
    presnterButtonsLayout->addWidget(stopPresentationButton,Qt::AlignRight);
    presnterButtonsLayout->addWidget(defaultPresentationButton,Qt::AlignRight);

    presnterButtonsLayout->setMargin(0);
    presnterButtonsLayout->setSizeConstraint(QLayout::SetMinimumSize);

    presenterButtonsBar->setLayout(presnterButtonsLayout);

    presenterButtonsBar->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    presenterButtonsBar->setAutoFillBackground(true);
    presenterButtonsBar->adjustSize();

    // Mainly for developmnent purposes
    presenterButtonsBar->setCursor(Qt::OpenHandCursor);
}

void threeButtonUI::nextSlideClicked(bool checked)
{
    qDebug() << "nextSlideClicked";

    overlayWindow *mw = (overlayWindow *)parentWidget;
    mw->sendPresntationControl(penNum, NEXT_SLIDE);
}


void threeButtonUI::prevSlideClicked(bool checked)
{
    qDebug() << "prevSlideClicked";

    overlayWindow *mw = (overlayWindow *)parentWidget;
    mw->sendPresntationControl(penNum, PREV_SLIDE);
}


void threeButtonUI::toggleLaserClicked(bool checked)
{
    qDebug() << "toggleLaserClicked";

    overlayWindow *mw = (overlayWindow *)parentWidget;
    mw->sendPresntationControl(penNum, SHOW_HIGHLIGHT);
}


void threeButtonUI::startPresentationClicked(bool checked)
{
    qDebug() << "startPresentationClicked";

    overlayWindow *mw = (overlayWindow *)parentWidget;
    mw->sendPresntationControl(penNum, START_SHOW);
}


void threeButtonUI::stopPresentationClicked(bool checked)
{
    qDebug() << "stopPresentationClicked";

    overlayWindow *mw = (overlayWindow *)parentWidget;
    mw->sendPresntationControl(penNum, STOP_SHOW);
}


void threeButtonUI::defaultPresentationClicked(bool checked)
{
    qDebug() << "defaultPresentationClicked";

    overlayWindow *mw = (overlayWindow *)parentWidget;
    mw->sendPresntationControl(penNum, START_DEFAULT);
}




void threeButtonUI::createMouseButtonArea(void)
{
    mouseButtonsBar                 = new QWidget(parentWidget);
    QHBoxLayout *mouseButtonsLayout = new QHBoxLayout();

    mouseButtonsLayout->setMargin(0);

    // The main three (with arrows as buttons too)
    middleMouseButton     = new QPushButton(parentWidget);
    rightMouseButton      = new QPushButton(parentWidget);
    //scrollUpMouseButton   = new QPushButton(parentWidget);
    //scrollDownMouseButton = new QPushButton(parentWidget);
    sendTextButton        = new QPushButton(parentWidget);

    setIconFromResource(middleMouseButton,    icons.middle_mouse,buttonWidth,buttonHeight);
    setIconFromResource(rightMouseButton,     icons.right_mouse,buttonWidth,buttonHeight);
    //setIconFromResource(scrollUpMouseButton,  ":/images/top/undo.svg",buttonWidth,buttonHeight);
    //setIconFromResource(scrollDownMouseButton,":/images/top/undo.svg",buttonWidth,buttonHeight);
    setIconFromResource(sendTextButton,       icons.text_entry,buttonWidth,buttonHeight);

    middleMouseButton->setFlat(true);
    rightMouseButton->setFlat(true);
    //scrollUpMouseButton->setFlat(true);
    //scrollDownMouseButton->setFlat(true);
    sendTextButton->setFlat(true);

    connect(middleMouseButton,      SIGNAL(clicked(bool)), this,SLOT(middleMouseClicked(bool)) );
    connect(rightMouseButton,       SIGNAL(clicked(bool)), this,SLOT(rightMouseClicked(bool)) );
    //connect(scrollUpMouseButton,    SIGNAL(clicked(bool)), this,SLOT(scrollUpMouseClicked(bool)) );
    //connect(scrollDownMouseButton,  SIGNAL(clicked(bool)), this,SLOT(scrollDownClicked(bool)) );
    connect(sendTextButton,         SIGNAL(clicked(bool)), this,SLOT(sendTextClicked(bool)) );

    mouseButtonsLayout->addWidget(middleMouseButton,Qt::AlignRight);
    mouseButtonsLayout->addWidget(rightMouseButton,Qt::AlignRight);
    //mouseButtonsLayout->addWidget(scrollUpMouseButton,Qt::AlignRight);
    //mouseButtonsLayout->addWidget(scrollDownMouseButton,Qt::AlignRight);
    mouseButtonsLayout->addWidget(sendTextButton,Qt::AlignRight);

    mouseButtonsLayout->setMargin(0);
    mouseButtonsLayout->setSizeConstraint(QLayout::SetMinimumSize);

    mouseButtonsBar->setLayout(mouseButtonsLayout);

    mouseButtonsBar->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    mouseButtonsBar->setAutoFillBackground(true);
    mouseButtonsBar->adjustSize();

    // Mainly for developmnent purposes
    mouseButtonsBar->setCursor(Qt::OpenHandCursor);
}

void threeButtonUI::middleMouseClicked(bool checked)
{
    qDebug() << "middleMouseClicked";

    overlayWindow *mw = (overlayWindow *)parentWidget;
    mw->setMouseButtonPulse(penNum, MOUSE_MIDDLE);
}

void threeButtonUI::rightMouseClicked(bool checked)
{
    qDebug() << "rightMouseClicked";

    overlayWindow *mw = (overlayWindow *)parentWidget;
    mw->setMouseButtonPulse(penNum, MOUSE_RIGHT);
}

void threeButtonUI::scrollUpMouseClicked(bool checked)
{
    qDebug() << "scrollUpMouseClicked";

    overlayWindow *mw = (overlayWindow *)parentWidget;
    mw->setMouseButtonPulse(penNum, MOUSE_SCROLLUP);
}

void threeButtonUI::scrollDownClicked(bool checked)
{
    qDebug() << "scrollDownClicked";

    overlayWindow *mw = (overlayWindow *)parentWidget;
    mw->setMouseButtonPulse(penNum, MOUSE_SCROLLDOWN);
}

void threeButtonUI::sendTextClicked(bool checked)
{
    qDebug() << "sendTextClicked";

    overlayWindow *mw = (overlayWindow *)parentWidget;

    keyboardAvailable = ! keyboardAvailable;

    mw->enableTextInputWindow(keyboardAvailable);
    if( keyboardAvailable )
        mw->setPenType(penNum, DRAW_TEXT);
    else
        mw->setPenType(penNum, DRAW_SOLID);
}

void threeButtonUI::createThreeButtonArea(void)
{
    qDebug() << "createThreeButtonArea";

    threeButtonBar         = new QWidget(parentWidget);
    QHBoxLayout *buttonBar = new QHBoxLayout();

    buttonBar->setMargin(0);

    // The main three (with arrows as buttons too)
    expandModesButton = new QPushButton(parentWidget);
    logoButton        = new QPushButton(parentWidget);
    logoPanZoomButton = new QPushButton(parentWidget);
    expandMenuButton  = new QPushButton(parentWidget);
    expandPrivateMenuButton = new QPushButton(parentWidget);
    screenSnapButton  = new QPushButton(parentWidget);

    setIconFromResource(expandModesButton,newIconResourceString,buttonWidth,buttonHeight,slide_left);
    setIconFromResource(logoButton,       icons.app_logo_closed,buttonWidth,buttonHeight);
    setIconFromResource(logoPanZoomButton,icons.app_logo_open,buttonWidth,buttonHeight);
    setIconFromResource(screenSnapButton, icons.snapshot, buttonWidth,buttonHeight);
    setIconFromResource(expandMenuButton, icons.annotate_mode,buttonWidth,buttonHeight,slide_down);
    setIconFromResource(expandPrivateMenuButton, icons.personal_notes,buttonWidth,buttonHeight,slide_down);

    expandMenuButton->setVisible(false);
    expandPrivateMenuButton->setVisible(false);

    expandModesButton->setFlat(true);
    logoButton->setFlat(true);
    logoPanZoomButton->setFlat(true);
    expandMenuButton->setFlat(true);
    expandPrivateMenuButton->setFlat(true);
    screenSnapButton->setFlat(true);

    connect(expandModesButton, SIGNAL(clicked(bool)), this,SLOT(modesClicked(bool)) );
    connect(logoButton,        SIGNAL(clicked(bool)), this,SLOT(logoClicked(bool)) );
    connect(logoPanZoomButton, SIGNAL(clicked(bool)), this,SLOT(logoClicked(bool)) );
    connect(expandMenuButton,  SIGNAL(clicked(bool)), this,SLOT(expandMenuClicked(bool)) );
    connect(expandPrivateMenuButton,  SIGNAL(clicked(bool)), this,SLOT(expandMenuClicked(bool)) );
    connect(screenSnapButton,  SIGNAL(clicked(bool)), this,SLOT(localScreenSnap(bool)) );

    buttonBar->addWidget(expandModesButton,Qt::AlignLeft);
    buttonBar->addWidget(logoButton,Qt::AlignLeft);
    buttonBar->addWidget(logoPanZoomButton,Qt::AlignLeft);
    buttonBar->addWidget(expandMenuButton,Qt::AlignLeft);
    buttonBar->addWidget(expandPrivateMenuButton,Qt::AlignLeft);
    buttonBar->addWidget(screenSnapButton,Qt::AlignLeft);

    buttonBar->setSizeConstraint(QLayout::SetMinimumSize);

    logoButton->show();
    logoPanZoomButton->hide();

    threeButtonBar->setLayout(buttonBar);
    threeButtonBar->setAutoFillBackground(true);

    threeButtonBar->adjustSize();

    // Mainly for developmnent purposes
    threeButtonBar->setCursor(Qt::OpenHandCursor);
}

void threeButtonUI::setExpandModeButtonsIconCurrentImage()
{
    METHOD_TRACE("setExpandModeButtonsIconCurrentImage currentMode =" << currentMode);

    QIcon icon;

    // TODO: distinguish between overlay, whiteboard & stickies modes
    switch( currentMode ) {
    case privateMode:
        newIconResourceString = icons.personal_notes;
        break;
    case overlayMode:
        newIconResourceString = icons.annotate_mode;
        break;
    case presenterMode:
        newIconResourceString = icons.presentation;
        break;
    case mouseMode:
        newIconResourceString = icons.mouse_mode;
        break;
    case stickiesMode:
        newIconResourceString = ":/images/old_files/stickies.svg";
        break;
    case whiteboardMode:
        newIconResourceString = ":/images/old_files/page/whiteboard.svg";
        break;
    case viewMode:
    case NODE_NOT_SET:
        newIconResourceString = icons.view_mode;
    }

    // Hack until we write the animator
    switch(modeButtonsState)
    {
    case mode_buttons_out:
    case mode_buttons_move_out:
        modeButtonsState = mode_buttons_out;
        setIconFromResource(expandModesButton,newIconResourceString,buttonWidth,buttonHeight,slide_right);
        expandModesButton->repaint();
        presenterButtonsBar->hide();
        modeButtonsBar->show();
        modeButtonsBar->move(((parentWidth-menuBuffer)-((threeButtonBar->width())+(modeButtonsBar->width()))),menuBuffer);
        //modeButtonsBar->move(threeButtonBar->x()-modeButtonsBar->width(), threeButtonY);
        parentWidget->repaint();
        break;

    case mode_buttons_move_in:
    case mode_buttons_in:
        modeButtonsState = mode_buttons_in;
        setIconFromResource(expandModesButton,newIconResourceString,buttonWidth,buttonHeight,slide_left);
        expandModesButton->repaint();
        modeButtonsBar->hide();
        parentWidget->repaint();
        break;
    }

    threeButtonBar->repaint();
    parentWidget->repaint();
    screenResized(parentWidth, parentHeight);
}

void threeButtonUI::modesClicked(bool checked)
{
    METHOD_TRACE("modesClicked");

    // Change to animation. The non move states are reached by the inimator only
    switch(modeButtonsState)
    {
    case mode_buttons_out:
    case mode_buttons_move_out:
        modeButtonsState = mode_buttons_move_in;
        break;

    case mode_buttons_move_in:
    case mode_buttons_in:
        modeButtonsState = mode_buttons_move_out;
        break;
    }

    buttonModeAnimator();

    parentWidget->repaint();
}

void threeButtonUI::buttonModeAnimator(void)
{
    // Start the animator if necessary
    if( ! modeButtonAnimatorRunning )
    {
        // We will do an animator later
    }

    setExpandModeButtonsIconCurrentImage();

    // Hack until we write the animator
    switch(modeButtonsState)
    {
    case mode_buttons_out:
    case mode_buttons_move_out:
        modeButtonsState = mode_buttons_out;
        setIconFromResource(expandModesButton,newIconResourceString,buttonWidth,buttonHeight,slide_right);
        expandModesButton->repaint();
        modeButtonsBar->show();
        if (currentMode == presenterMode) {presenterButtonsBar->hide();}
        if (currentMode == mouseMode) {mouseButtonsBar->hide();}
        modeButtonsBar->move(((parentWidth-menuBuffer)-((threeButtonBar->width())+(modeButtonsBar->width()))),menuBuffer);
        parentWidget->repaint();
        break;

    case mode_buttons_move_in:
    case mode_buttons_in:
        modeButtonsState = mode_buttons_in;
        setIconFromResource(expandModesButton,newIconResourceString,buttonWidth,buttonHeight,slide_left);
        expandModesButton->repaint();
        modeButtonsBar->hide();
        if (currentMode == presenterMode) {presenterButtonsBar->show();}
        if (currentMode == mouseMode) {mouseButtonsBar->show();}
        parentWidget->repaint();
        break;
    }
}

void threeButtonUI::logoClicked(bool checked)
{
    if( busyMode ) { METHOD_TRACE("logoClicked: but busyMode"); return; }

    METHOD_TRACE("logoClicked");

    overlayWindow *mw = (overlayWindow *)parentWidget;

    if( logoButton->isVisible() )
    {
        // Enter pan/zoom mode
        mw->setZoomPanModeOn(true);

        logoButton->hide();
        logoPanZoomButton->show();
        screenSnapButton->show();
        expandModesButton->hide();
        expandMenuButton->hide();
        expandPrivateMenuButton->hide();

        logoPanZoomButton->repaint();
        screenSnapButton->repaint();

        modeButtonsBar->hide();
        menuButtonsArea->hide();
        privateMenuButtonsArea->hide();
        currentTabSwitch(tabs_hidden);
        presenterButtonsBar->hide();
        mouseButtonsBar->hide();

        threeButtonBar->adjustSize();
        threeButtonBar->move(parentWidth-(threeButtonBar->width())-menuBuffer,menuBuffer);
    }
    else if (currentMode == viewMode)
    {
        logoButton->show();
        logoPanZoomButton->hide();
        expandModesButton->show();

        logoPanZoomButton->repaint();

        threeButtonBar->adjustSize();
        threeButtonBar->move(parentWidth-(threeButtonBar->width())-menuBuffer,menuBuffer);
    }
    else
    {
        // Out of pan/zoom mode
        mw->setZoomPanModeOn(false);

        logoButton->show();
        logoPanZoomButton->hide();
        expandModesButton->show();

        logoPanZoomButton->repaint();

        if( currentMode == overlayMode )
        {
            expandMenuButton->show();
            //menuButtonsArea->show();
            currentTabSwitch(tab_main_controls);
        }
        if(currentMode == privateMode)
        {
            expandPrivateMenuButton->show();
            currentTabSwitch(tab_p_main_controls);
        }

        if( currentMode == mouseMode )     mouseButtonsBar->show();
        if( currentMode == presenterMode ) presenterButtonsBar->show();

        threeButtonBar->adjustSize();
        threeButtonBar->move(parentWidth-(threeButtonBar->width())-menuBuffer,menuBuffer);
    }

    parentWidget->repaint();
}

void threeButtonUI::expandMenuClicked(bool checked)
{
    METHOD_TRACE("expandMenuClicked");

    if( currentTab != tabs_hidden || menuButtonsArea->isVisible() )
    {
        menuButtonsArea->hide();
        privateMenuButtonsArea->hide();
        currentTabSwitch(tabs_hidden);
    }
    else
    {
        if(currentMode == overlayMode)
        {
            menuButtonsArea->show();
            currentTabSwitch(tab_main_controls);
        }
        if(currentMode == privateMode)
        {
            privateMenuButtonsArea->show();
            currentTabSwitch(tab_p_main_controls);
        }
    }
}

void threeButtonUI::setIconFromResource(QPushButton *button, QString res, int iconWidth, int iconHeight)
{
    // If nothing to do...
    if( iconWidth < 1 || iconHeight < 1 ) return;

    QPixmap iconPixmap;

    QImage  iconImage(iconWidth,iconHeight, QImage::Format_ARGB32_Premultiplied);
    iconImage.fill(Qt::transparent);

    QSvgRenderer re;
    QPainter iconPaint(&iconImage);
    iconPaint.setRenderHint(QPainter::Antialiasing);

    re.load(res);

    // Do we scale one of the dimensions down as it's not
    QRectF imageDim = iconImage.rect();
    double xScale   = imageDim.width()  / ((double)iconWidth);
    double yScale   = imageDim.height() / ((double)iconHeight);
    QRectF bounds;
    if( xScale >= yScale ) {
        // X is limiting the scale
        bounds = QRectF(0,0,xScale*imageDim.width(),xScale*imageDim.height()); // left, top, width, height
    }
    else {
        // Y is limiting the scale
        bounds = QRectF(0,0,yScale*imageDim.width(),yScale*imageDim.height()); // left, top, width, height
    }
    re.render(&iconPaint,bounds);

    qDebug() << "Button image bounds:" << bounds << "resource:" << res;

    iconPaint.end();

    iconPixmap = QPixmap::fromImage(iconImage);

    button->setFlat(true);
    button->setIcon(iconPixmap);
    button->setIconSize(QSize(iconWidth,iconHeight));
    button->setFixedWidth(iconWidth);
    button->setFixedHeight(iconHeight);
}

void threeButtonUI::setIconFromResource(QPushButton *button, QString res,
                                        int iconWidth, int iconHeight,
                                        slide_icon_type_t arrowType)
{
    // If nothing to do...
    if( iconWidth < 1 || iconHeight < 1 ) return;

    QString arrowRes;

    double  mainIconTop, mainIconLeft, mainIconWidth, mainIconHeight;
    double  arrowTop,    arrowLeft,    arrowWidth,    arrowHeight;

    switch( arrowType )
    {
    case slide_left:  arrowRes = ":/images/old_files/left_slide_control.svg";
                      mainIconLeft   = iconWidth/4.0;
                      mainIconTop    = 0.0;
                      mainIconWidth  = iconWidth - iconWidth/4.0;
                      mainIconHeight = iconWidth;
                      arrowLeft   = 0.0;
                      arrowTop    = 0.0;
                      arrowWidth  = iconWidth/5.0;
                      arrowHeight = iconWidth;
                      break;
    case slide_right: arrowRes = ":/images/old_files/right_slide_control.svg";
                      mainIconLeft   = 0.0;
                      mainIconTop    = 0.0;
                      mainIconWidth  = iconWidth - iconWidth/4.0;
                      mainIconHeight = iconWidth;
                      arrowLeft   = iconWidth - iconWidth/5.0;
                      arrowTop    = 0.0;
                      arrowWidth  = iconWidth/5.0;
                      arrowHeight = iconWidth;
                      break;
    case slide_up:    arrowRes = ":/images/old_files/up_slide_control.svg";
                      mainIconLeft   = 0.0;
                      mainIconTop    = iconWidth/4.0;
                      mainIconWidth  = iconWidth;
                      mainIconHeight = iconWidth - iconWidth/4.0;
                      arrowLeft   = 0.0;
                      arrowTop    = 0.0;
                      arrowWidth  = iconWidth;
                      arrowHeight = iconWidth/5.0;
                      break;
    case slide_down:  arrowRes = ":/images/old_files/down_slide_control.svg";
                      mainIconLeft   = 0.0;
                      mainIconTop    = 0.0;
                      mainIconWidth  = iconWidth;
                      mainIconHeight = iconWidth - iconWidth/4.0;
                      arrowLeft   = 0.0;
                      arrowTop    = iconWidth - iconWidth/5.0;
                      arrowWidth  = iconWidth;
                      arrowHeight = iconWidth/5.0;
                      break;

    default:          qDebug() << "threeButtonUI: bad arrow type:" << arrowType;
                      return;
    }

    // Set up the overall icon
    QPixmap iconPixmap;

    QImage  iconImage(iconWidth,iconHeight, QImage::Format_ARGB32_Premultiplied);
    iconImage.fill(Qt::transparent);

    QSvgRenderer re;
    QPainter iconPaint(&iconImage);
    iconPaint.setRenderHint(QPainter::Antialiasing);
    QRectF imageDim = iconImage.rect();

    // Main icon
    QRectF bounds;

    double xScale   = mainIconWidth / imageDim.width();
    double yScale   = mainIconHeight / imageDim.height();

    double newWidth, newHeight;

    if( xScale >= yScale ) { // X is limiting the scale
        newWidth  = yScale*imageDim.width();
        newHeight = yScale*imageDim.height();
        qDebug() << "threeButtonUI: xScale>=yScale, newWidth =" << newWidth << "newHeight =" << newHeight;
    }
    else { // Y is limiting the scale
        newWidth  = xScale*imageDim.width();
        newHeight = xScale*imageDim.height();
        qDebug() << "threeButtonUI: xScale<yScale, newWidth =" << newWidth << "newHeight =" << newHeight;
    }

    // Center up again
    if( newWidth  < mainIconWidth )  mainIconLeft += (mainIconWidth  - newWidth) / 2.0;
    if( newHeight < mainIconHeight ) mainIconTop  += (mainIconHeight - newHeight) / 2.0;

    bounds = QRectF(mainIconLeft,mainIconTop,newWidth,newHeight);

    qDebug() << "Button image bounds:" << bounds << "resource:" << res;

    re.load(res);
    re.render(&iconPaint,bounds);

    // Arrow icon
    bounds = QRectF(arrowLeft,arrowTop,arrowWidth,arrowHeight); // left, top, width, height

    qDebug() << "setIconFromResource: arrow bounds:" << bounds;

    re.load(arrowRes);
    re.render(&iconPaint,bounds);

    iconPaint.end();

    iconPixmap = QPixmap::fromImage(iconImage);

    button->setFlat(true);
    button->setIcon(iconPixmap);
    button->setIconSize(QSize(iconWidth,iconHeight));
    button->setFixedWidth(iconWidth);
    button->setFixedHeight(iconHeight);
}


// Interface

void threeButtonUI::screenResized(int newWidth, int newHeight)
{
    METHOD_TRACE("threeButtonUI: set screen size to:" << newWidth << newHeight);

    parentWidth  = newWidth;
    parentHeight = newHeight;

    if( (buttonHeight * (3 + 1 + 3)) > parentHeight )
    {
        // Small display positioning
        threeButtonY = buttonHeight / 4;
        menuAreaY    = threeButtonY + buttonHeight + buttonHeight / 4;
    }
    else
    {
        // Standard positions
        threeButtonY = 1*buttonHeight;
        menuAreaY    = 3*buttonHeight;
    }

    //The X & Y of the top 3 button menu
    threeButtonBar->move(parentWidth-(threeButtonBar->width())-menuBuffer,menuBuffer);

    //The X & Y of the mode selection sub-menu
    modeButtonsBar->move(((parentWidth-menuBuffer)-((threeButtonBar->width())+
                                                    (modeButtonsBar->width()))),menuBuffer);

    //The X, Y & size of the annotation mode sub-menu
    //the buttonWidth mulipler screws up depending on device on Galaxy A6 (3*buttonWith) & on Galaxy S4 (7*buttonWidth)
    menuButtonsArea->setGeometry(threeButtonBar->x()-(3*buttonWidth),threeButtonBar->y()+(menuAreaY-(menuAreaY/2)),
                                 7*buttonWidth, (1+rowsDisplayed)*buttonHeight);

    //The X, Y & size of the private notes mode sub-menu
    //the buttonWidth mulipler screws up depending on device on Galaxy A6 (3*buttonWith) & on Galaxy S4 (7*buttonWidth)
    privateMenuButtonsArea->setGeometry(threeButtonBar->x()-(3*buttonWidth),threeButtonBar->y()+(menuAreaY-(menuAreaY/2)),
                                 7*buttonWidth, (1+rowsDisplayed)*buttonHeight);

    //The X, Y & size of the notification window
    notificationArea->setGeometry(threeButtonBar->x()-3*buttonWidth,threeButtonBar->y()+2*buttonHeight,
                                  5*buttonWidth, 3*buttonHeight);

    //The X & Y of the mouse mode sub-menu
    mouseButtonsBar->move(((parentWidth-menuBuffer)-((threeButtonBar->width())+
                                                 (mouseButtonsBar->width()))),menuBuffer);

    //The X & Y of the presentation mode sub-menu
    presenterButtonsBar->move(((parentWidth-menuBuffer)-((threeButtonBar->width())+
                                                         (presenterButtonsBar->width()))),menuBuffer);
}


void threeButtonUI::setSessionType(int newSessionType)
{
    METHOD_TRACE("threeButtonUI: setSessionType:" << newSessionType
                 << "(generally, this means that the requested state change has happened)");

    displayMessage(QString("setSessionType(%1)").arg(newSessionType), 5000);

    uiLockedUntilServerActionComplete = false;

    if( newSessionType == currentSessionType ) return;

    currentSessionType = newSessionType;

    setExpandModeButtonsIconCurrentImage();
}

// Hide everything except the cosneta logo and TODO: start it rotating
void threeButtonUI::setBusyMode(void)
{
    METHOD_TRACE("setBusyMode");

    // Set flag (prevents clicks on buttons doing anything)
    busyMode = true;

    // Hide everything except the cosneta logo
    expandModesButton->hide();
    logoButton->show();
    logoPanZoomButton->hide();
    expandMenuButton->hide();
    expandPrivateMenuButton->hide();
    parentWidget->repaint();
    threeButtonBar->repaint();

    modeButtonsBar->hide();
    mouseButtonsBar->hide();
    presenterButtonsBar->hide();
    menuButtonsArea->hide();
    privateMenuButtonsArea->hide();
    currentTabSwitch(tabs_hidden);
    // NB leave notificationArea as it is still in use

    // TODO: Start the logo rotating
}

void threeButtonUI::endBusyMode(void)
{
    METHOD_TRACE("endBusyMode");

    // Clear flag (prevents clicks on buttons doing anything)
    busyMode = false;

    // Show the modes button again
    expandModesButton->show();
    logoButton->show();
    expandMenuButton->hide();
    threeButtonBar->repaint();
    presenterButtonsBar->repaint();
    parentWidget->repaint();

    // TODO: Stop the logo rotating
}

void threeButtonUI::setViewMode(void)
{
    currentSessionType = 0;
    remoteEditMode     = false;
    currentMode        = viewMode;
    menuButtonsArea->hide();
    privateMenuButtonsArea->hide();
    currentTabSwitch(tabs_hidden);
    setExpandModeButtonsIconCurrentImage();
    endBusyMode();
}


void threeButtonUI::setMenuMode(current_menu_t newMode)
{
    qDebug() << "setMenuMode() to" << newMode;

    // Never called at present...
//    currentMode = newMode;

    setExpandModeButtonsIconCurrentImage();

    endBusyMode();
}

int threeButtonUI::getMenuMode(void)
{
    return currentMode;
}

void threeButtonUI::makePeer(void)
{
    METHOD_TRACE("makePeer");
}

void threeButtonUI::makeSessionLeader(void)
{
    qDebug() << "makeSessionLeader";
}

void threeButtonUI::makeFollower(void)
{
    qDebug() << "makeFollower";
}

// Show progress by rotating the Cosneta logo. or stop it.
void threeButtonUI::sessionStateChangeAnimate(void)
{
    METHOD_TRACE("Start logo rotating - and lock UI");

    modeButtonsBar->hide();

    parentWidget->repaint();
}

void threeButtonUI::sessionStateChangeComplete(void)
{
    METHOD_TRACE("Stop logo rotating - and unlock UI");

    parentWidget->repaint();
}





void threeButtonUI::hide(void)
{
    threeButtonBar->hide();
    modeButtonsBar->hide();
    mouseButtonsBar->hide();
    notificationArea->hide();
    presenterButtonsBar->hide();
    menuButtonsArea->hide();
    privateMenuButtonsArea->hide();
    currentTabSwitch(tabs_hidden);
}

void threeButtonUI::show(void)
{
    threeButtonBar->show();
    modeButtonsBar->hide();
    mouseButtonsBar->hide();
    presenterButtonsBar->hide();
    menuButtonsArea->hide();
    privateMenuButtonsArea->hide();
    currentTabSwitch(tabs_hidden);
}

bool threeButtonUI::isVisible(void)
{
    return threeButtonBar->isVisible();
}

void threeButtonUI::raise(void)
{
    modeButtonsBar->raise();
    threeButtonBar->raise();
    menuButtonsArea->raise();
    privateMenuButtonsArea->raise();
    /*switch(currentTab)
    {
        case tab_sticky:
            stickyTabButtonsArea->raise();
        break;
        case tab_undo:
            undoTabButtonsArea->raise();
        break;
        case tab_main_controls:
            mainControlsTabButtonsArea->raise();
        break;
        case tab_shapes:
            penShapesTabButtonsArea->raise();
        break;
        case tab_page_controls:
            pageControlsTabButtonsArea->raise();
        break;
        case tab_capture:
            captureTabButtonsArea->raise();
        break;
    }*/
    mouseButtonsBar->raise();
    presenterButtonsBar->raise();

    // Notificati9on area at end so it is on top of everything else
    notificationArea->raise();
}

QPoint threeButtonUI::pos(void)
{
    int minX = 0;
    int minY = 0;

    if( threeButtonBar->isVisible() )
    {
        minX = threeButtonBar->x();
        minY = threeButtonBar->y();
    }
    else
    {
        qDebug() << "threeButtonUI::pos() ERROR: the three button area must always be visible.";
    }

    if( modeButtonsBar->isVisible() )
    {
        if( minX > modeButtonsBar->x() ) minX = modeButtonsBar->x();
        if( minY > modeButtonsBar->y() ) minY = modeButtonsBar->y();
    }

    if( mouseButtonsBar->isVisible() )
    {
        if( minX > mouseButtonsBar->x() ) minX = mouseButtonsBar->x();
        if( minY > mouseButtonsBar->y() ) minY = mouseButtonsBar->y();
    }

    if( presenterButtonsBar->isVisible() )
    {
        if( minX > presenterButtonsBar->x() ) minX = presenterButtonsBar->x();
        if( minY > presenterButtonsBar->y() ) minY = presenterButtonsBar->y();
    }

    if( menuButtonsArea->isVisible() )
    {
        if( minX > menuButtonsArea->x() ) minX = menuButtonsArea->x();
        if( minY > menuButtonsArea->y() ) minY = menuButtonsArea->y();
    }
    if( privateMenuButtonsArea->isVisible() )
    {
        if( minX > privateMenuButtonsArea->x() ) minX = privateMenuButtonsArea->x();
        if( minY > privateMenuButtonsArea->y() ) minY = privateMenuButtonsArea->y();
    }
/*
    if( stickyTabButtonsArea->isVisible() )
    {
        if( minX > stickyTabButtonsArea->x() ) minX = stickyTabButtonsArea->x();
        if( minY > stickyTabButtonsArea->y() ) minY = stickyTabButtonsArea->y();
    }

    if( undoTabButtonsArea->isVisible() )
    {
        if( minX > undoTabButtonsArea->x() ) minX = undoTabButtonsArea->x();
        if( minY > undoTabButtonsArea->y() ) minY = undoTabButtonsArea->y();
    }

    if( mainControlsTabButtonsArea->isVisible() )
    {
        if( minX > mainControlsTabButtonsArea->x() ) minX = mainControlsTabButtonsArea->x();
        if( minY > mainControlsTabButtonsArea->y() ) minY = mainControlsTabButtonsArea->y();
    }

    if( penShapesTabButtonsArea->isVisible() )
    {
        if( minX > penShapesTabButtonsArea->x() ) minX = penShapesTabButtonsArea->x();
        if( minY > penShapesTabButtonsArea->y() ) minY = penShapesTabButtonsArea->y();
    }

    if( pageControlsTabButtonsArea->isVisible() )
    {
        if( minX > pageControlsTabButtonsArea->x() ) minX = pageControlsTabButtonsArea->x();
        if( minY > pageControlsTabButtonsArea->y() ) minY = pageControlsTabButtonsArea->y();
    }

    if( captureTabButtonsArea->isVisible() )
    {
        if( minX > captureTabButtonsArea->x() ) minX = captureTabButtonsArea->x();
        if( minY > captureTabButtonsArea->y() ) minY = captureTabButtonsArea->y();
    }*/

    return QPoint(minX,minY);
}

QSize threeButtonUI::size(void)
{
    QPoint origin = pos();
    int    maxX   = origin.x();
    int    maxY   = origin.y();

    if( threeButtonBar->isVisible() )
    {
        maxX = threeButtonBar->x() + threeButtonBar->width();
        maxY = threeButtonBar->y() + threeButtonBar->height();
    }
    else
    {
        qDebug() << "threeButtonUI::size() ERROR: the three button area must always be visible.";
    }

    if( modeButtonsBar->isVisible() )
    {
        int modMaxX = modeButtonsBar->x() + modeButtonsBar->width();
        int modMaxY = modeButtonsBar->y() + modeButtonsBar->height();

        if( maxX < modMaxX ) maxX = modMaxX;
        if( maxY < modMaxY ) maxY = modMaxY;
    }

    if( mouseButtonsBar->isVisible() )
    {
        int mouseMaxX = mouseButtonsBar->x() + mouseButtonsBar->width();
        int mouseMaxY = mouseButtonsBar->y() + mouseButtonsBar->height();

        if( maxX < mouseMaxX ) maxX = mouseMaxX;
        if( maxY < mouseMaxY ) maxY = mouseMaxY;
    }

    if( presenterButtonsBar->isVisible() )
    {
        int mouseMaxX = presenterButtonsBar->x() + presenterButtonsBar->width();
        int mouseMaxY = presenterButtonsBar->y() + presenterButtonsBar->height();

        if( maxX < mouseMaxX ) maxX = mouseMaxX;
        if( maxY < mouseMaxY ) maxY = mouseMaxY;
    }

    if( menuButtonsArea->isVisible() )
    {
        int menuMaxX = menuButtonsArea->x() + menuButtonsArea->width();
        int menuMaxY = menuButtonsArea->y() + menuButtonsArea->height();

        if( maxX < menuMaxX ) maxX = menuMaxX;
        if( maxY < menuMaxY ) maxY = menuMaxY;
    }

    if( privateMenuButtonsArea->isVisible() )
    {
        int pMenuMaxX = privateMenuButtonsArea->x() + privateMenuButtonsArea->width();
        int pMenuMaxY = privateMenuButtonsArea->y() + privateMenuButtonsArea->height();

        if( maxX < pMenuMaxX ) maxX = pMenuMaxX;
        if( maxY < pMenuMaxY ) maxY = pMenuMaxY;
    }

    /*if( stickyTabButtonsArea->isVisible() )
    {
        int menuMaxX = stickyTabButtonsArea->x() + stickyTabButtonsArea->width();
        int menuMaxY = stickyTabButtonsArea->y() + stickyTabButtonsArea->height();

        if( maxX < menuMaxX ) maxX = menuMaxX;
        if( maxY < menuMaxY ) maxY = menuMaxY;
    }

    if( undoTabButtonsArea->isVisible() )
    {
        int menuMaxX = undoTabButtonsArea->x() + undoTabButtonsArea->width();
        int menuMaxY = undoTabButtonsArea->y() + undoTabButtonsArea->height();

        if( maxX < menuMaxX ) maxX = menuMaxX;
        if( maxY < menuMaxY ) maxY = menuMaxY;
    }

    if( mainControlsTabButtonsArea->isVisible() )
    {
        int menuMaxX = mainControlsTabButtonsArea->x() + mainControlsTabButtonsArea->width();
        int menuMaxY = mainControlsTabButtonsArea->y() + mainControlsTabButtonsArea->height();

        if( maxX < menuMaxX ) maxX = menuMaxX;
        if( maxY < menuMaxY ) maxY = menuMaxY;
    }

    if( penShapesTabButtonsArea->isVisible() )
    {
        int menuMaxX = penShapesTabButtonsArea->x() + penShapesTabButtonsArea->width();
        int menuMaxY = penShapesTabButtonsArea->y() + penShapesTabButtonsArea->height();

        if( maxX < menuMaxX ) maxX = menuMaxX;
        if( maxY < menuMaxY ) maxY = menuMaxY;
    }

    if( pageControlsTabButtonsArea->isVisible() )
    {
        int menuMaxX = pageControlsTabButtonsArea->x() + pageControlsTabButtonsArea->width();
        int menuMaxY = pageControlsTabButtonsArea->y() + pageControlsTabButtonsArea->height();

        if( maxX < menuMaxX ) maxX = menuMaxX;
        if( maxY < menuMaxY ) maxY = menuMaxY;
    }

    if( captureTabButtonsArea->isVisible() )
    {
        int menuMaxX = captureTabButtonsArea->x() + captureTabButtonsArea->width();
        int menuMaxY = captureTabButtonsArea->y() + captureTabButtonsArea->height();

        if( maxX < menuMaxX ) maxX = menuMaxX;
        if( maxY < menuMaxY ) maxY = menuMaxY;
    }*/

    return QSize(maxX-origin.x(),maxY-origin.y());
}
