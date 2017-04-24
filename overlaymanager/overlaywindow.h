#ifndef OVERLAYWINDOW_H
#define OVERLAYWINDOW_H

#include <QWidget>
#include <QImage>
#include <QTimer>
#include <QFileDialog>
#include <QDesktopWidget>
#include <QLineEdit>
#include <QtNetwork>
#include <QCheckBox>
#include <QSystemTrayIcon>
#include <QLabel>
#include <QMenu>
#include <QDateTime>
#include <QDir>

#include "build_options.h"

#ifdef TABLET_MODE
#include <QMainWindow>
#include <QTouchEvent>
#endif


#include <QVector>

#include "screenGrabber.h"
#include "overlay.h"
#include "networkClient.h"
#ifndef TABLET_MODE
#include "viewBroadcaster.h"
#endif
#include "menuStructure.h"
#include "buttonStrip.h"
#include "threeButtonUI.h"
#include "penCursor.h"
#include "stickyNote.h"
#include "replayManager.h"
#include "remoteConnectDialogue.h"
#include "localDrawTrace.h"
#include "cosnetaAPI.h"
#include "../systrayqt/net_connections.h"

#define INVALID_STICKY_NUM -1
#define NUM_CHAR_REPEATS    5

class QAction;

typedef enum { NO_MENU, MAIN_MENU, PEN_MENU } menuSelState_t;
typedef enum
{
    FD_SCREENSNAP,
    FD_RECORD_SEL,
    FD_REPLAY_SEL,
    FD_NO_ACTION

} fileDialogueAction_t;


#ifdef TABLET_MODE
class overlayWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit overlayWindow(bool sysTrayIsRunning, QMainWindow *parent = 0);

#else
class overlayWindow : public QWidget
{
    Q_OBJECT

public:
    overlayWindow(bool sysTrayIsRunning, QWidget *parent = 0);
#endif
    ~overlayWindow();

    void    hostedModeToggle(int requestingPen);
    void    setPenShape(int penNum, shape_t shape);
    void    setPenType(int penNum, penAction_t type);
    void    remoteNetSelectAndConnect(int penNum);
    void    addNewStickyStart(int penNum);
    void    allowRemoteNetSession(int penNum);
    int     currentSessionType(void);
    void    setZoomPanModeOn(bool on);
    void    setPenMode(int penNum, int newMode);
    void    setMouseButtonPulse(int penNum, fakeMouseButtonAction_t action);
    void    sendPresntationControl(int penNum, presentation_controls_t action);
    void    enableTextInputWindow(bool on);
    QColor  getLocalPenColour(void);


    bool           brushAction[MAX_SCREEN_PENS];
    penCursor     *cursor[MAX_SCREEN_PENS];            // Parented to overlay
    int            width, height;  // Current screen overlay dimensions

protected:
    void       paintEvent(QPaintEvent *event);

    void       mouseMoveEvent(QMouseEvent *e);  // These allow us to
    void       mousePressEvent(QMouseEvent *e);
    void       mouseReleaseEvent(QMouseEvent *e);
    void       wheelEvent(QWheelEvent *w);
    void       keyPressEvent(QKeyEvent *keyEv);      // Allow zoom/pan with keyboard
    void       keyReleaseEvent(QKeyEvent *keyEv);
    void       reShowRemoteConnectDialogue(void);
#ifdef TABLET_MODE
    bool       event(QEvent *e);
#endif


public slots:
    void       mainMenuQuit(void);
    void       mainMenuDisconnect(void);
    void       sendRemoteDebug(void);
    void       sendRemoteDebugDone(QNetworkReply *reply);
// DO NOT USE DEFINES as qmake doesn't pick up the build state and so never uses the code
//#ifdef TABLET_MODE
    void       textInputSend(bool checked);
    void       textInputReturnPressed(bool checked);
    void       textInputDeletePressed(bool checked);
//#endif
    void       setRate250();
    void       setRate500();
    void       setRate750();
    void       setRate1000();
    void       setRate1500();
    void       setRate2000();
    void       setRate4000();
    void       setRate6000();

    void       disableWhiteboard();
    void       fileDialogAccept();
    void       fileDialogReject();
    void       remoteSelected();
    void       remoteConnect();
    void       remoteConnectFailed();
    bool       connectToRemoteForEdit();
    bool       disconnectFromRemoteEdit();
    void       remoteResolutionChanged(int w, int h);
    void       netClientUpdate();
    void       movePenFromReceivedData(int penNum, int x, int y, bool show);
    void       toggleMouseAsPen();
    void       penDisconnectedOnHost();
    void       remoteViewCanBeShown();

private:
    // Methods
    QImage    *imageOfDrawnText( QString text );
    QImage    *imageOfJogShuttle(int screenWidth, bool showPause);
    QImage    *imageOfPageSelect(void);
    void       renderPen(QImage *img, QColor col);
    void       resetPens(void);
    void       waitMS(int msec);
    void       setUpDialogueCursors(QWidget *parent);
    void       playbackRewind(void);
    void       playbackStop(void);
    void       playbackTogglePlayPause(void);
    void       activePenUpdate(pen_state_t *state, int penNum, int xLoc[], int yLoc[]);
    bool       addNewSticky(int penNum, int stickySize, QPoint pos);
    void       leaveStickyMode(void);
    void       sendConnectedPenDataToHost(pen_state_t &state);
    void       getPenData(int penNum, pen_state_t &state);
    void       normaliseZoomPan(int rmtViewWidthScaled, int rmtViewHeightScaled);
#ifdef TABLET_MODE
    void       pinchZoom(QPointF a, QPointF b, bool first);
#endif
    void       createTextInputWindow(void);
    void       toggleNewRate(QAction *newAction);
    void       notifyAndroidMediaScanner(QString filename);

    bool       useMouseAsInputToo;
    int        mouseX, mouseY;
    int        mouseButtons;
    int        wheelPulseLength;
    int        wheelDeltaAccumulate;
    int        keyPress;
//    int        keyPressModifiers;
    uint32_t   sendSequenceNumber;
    uint32_t   currentSendAppCommand;
    uint32_t   keypressSequenceNumber;
    uint32_t   lastKeypressSequenceNumber;
    uint32_t   menuCommand;
    uint32_t   menuSequenceNumber;
    uint32_t   lastMenuSequenceNumber;
    uint32_t   presentationCommand;
    uint32_t   presentationSequenceNumber;
    uint32_t   lastPresentationSequenceNumber;
    int        mouseButtonsPulse;
    int        mouseButtonsPulseCount;
    int        modeSendRepeat;
    int        modeSetCommand;
    QVector<int>  sendCharacters;
    int           lastSendCharRepeat;

    // Data
    QFileDialog         *fileDialogue;
    fileDialogueAction_t fileDialogueAction;
    QPixmap              screenSnapData;

    remoteConnectDialogue *remoteSelector;
    QWidget             *textInputDialogue;
    QLineEdit           *arbritraryTextInput;

    int                  sessionType;

    bool                 displayMenuButtonStrip;

    bool                 connectedToViewHost;
    int                  netClientReceivedWords;
    int                  lastAckedReceivedWords;

    networkClient       *netClient;
#ifndef TABLET_MODE
    viewBroadcaster     *broadcast;
#endif
    QUdpSocket           remoteJoinSkt;
    QHostAddress         hostAddr;
    int                  hostPort;
    bool                 remoteConnection;
    bool                 viewOnlyMode;
    bool                 privOnlyMode;                                                   //pn
    int                  penConnectedToRemote; // May change to all local pens
    int                  penNumOnRemoteSystem;
    net_message_t        dataPacket;

    screenGrabber        *screenGrab;

    QThread              viewReceiverThread;
    overlay              *annotations;
    QImage               *annotationsImage;
    QImage               *pAnnotationsImage;
    QImage               *highlights;
    QImage               *replayBackgroundImage;
    QImage               *textLayerImage;
    localDrawTrace       *drawTrace;
    //privateNoteDrawTrace *pNoteDrawTrace;

    QDesktopWidget *desktop;
    int             numScreens;     // Currently only support overlay on a single screen, but if a
                                    // new one appears, give the user the option to move this there.
    float           nonLocalViewScale;
    float           nonLocalViewScaleMax;
    int             nonLocalViewCentreX;
    int             nonLocalViewCentreY;
    bool            inTouchDrag;
    QPointF         touchDragStart;
    bool            inPinchZoom;
    float           pinchStartSeparation;
    QPointF         pinchStartMidpoint;
    float           pinchStartScale;
    int             pinchStartCentreX;
    int             pinchStartCentreY;

    bool            localControlIsZoomPan;

    QColor         defaultOpaqueBackgroundColour;

    QImage        *titleImage;
    QPoint         titlePos;
    bool           showActionImage;

    bool           showIntroBackground;

    QImage        *jogShuttleImage;
    QRectF         jogShuttlePos;

    // Stickies
    bool                addingNewStickyMode[MAX_SCREEN_PENS];
    int                 newStickySize[MAX_SCREEN_PENS];
    QList<stickyNote *> notes;
    int                 penStickyNumber[MAX_SCREEN_PENS];

    bool                pageSelectActive;
    int                 firstShownThumbNail;
    QImage             *pageSelectImage;
    int                 currentPageNumber;
    int                 previewWidth;
    int                 entryWidth;
    bool                pageSelectLeftArrow;
    bool                pageSelectRightArrow;
    QVector<QImage>     pagePreview;

    class menuStructure *penMenu[MAX_SCREEN_PENS];
#ifdef USE_BUTTON_STRIP
    buttonStrip         *menuButtons;
#endif
#ifdef USE_THREE_BUTTON_UI
    threeButtonUI       *threeButtonInterface;
#endif
    QTimer        *timer;
    QTimer        *playbackRefire;
    bool           penWasActive[MAX_SCREEN_PENS];
    int            penLastMode[MAX_SCREEN_PENS];
    bool           penIsDrawing[MAX_SCREEN_PENS];
    bool           waitForPenButtonsRelease[MAX_SCREEN_PENS];
    bool           penInTextMode[MAX_SCREEN_PENS];
    uint32_t       lastAppCtrlSequenceNum[MAX_SCREEN_PENS];
    uint32_t       lastButtons[MAX_SCREEN_PENS];
    QColor         defaultPenColour[MAX_SCREEN_PENS];       // Do we need to re-render the pen?
    penCursor     *dialogCursor[MAX_SCREEN_PENS];      // Parented to dialog(s) - reparent
    bool           playingRecordedSession;
    bool           playingBackPaused;
    bool           deleteStickyOnClick[MAX_SCREEN_PENS];
    bool           startMoveOnStickyClick[MAX_SCREEN_PENS];
    bool           penIsDraggingSticky[MAX_SCREEN_PENS];
    bool           dragStickyEndOnRelease[MAX_SCREEN_PENS];
    stickyNote    *stickyBeingUpdatedByPenNum[MAX_SCREEN_PENS];

    bool           inHostedMode;
    bool           sysTrayDetected;

    pen_settings_t penSettings[MAX_SCREEN_PENS];       // Settings retreived from the API

    replayManager *replay;

private slots:
    void     readPenData(void);
    void     playbackNextStep(void);
    void     trayIconClicked(QSystemTrayIcon::ActivationReason reason);
//    void     screenResized(int newWidth, int newHeight);
//    void     numScreensChanged(int newNumScreens);
    void     help();

public:
    void     newPage(void);
    void     nextPage(void);
    void     prevPage(void);
    void     showPageSelector(void);
    void     clearOverlay(void);
    void     clearPrivateNotes(void);
    void     setTextPosFromPixelPos(int penNum, int x, int y);
    void     addTextCharacterToOverlay(int penNum, quint32 charData);
    void     applyReceivedMenuCommand(int penNum, quint32 menuCommand);
    void     paperColourChanged(int penNum, QColor newColour);
    void     privPaperColourChanged(QColor newColour);
    void     paperColourTransparent(int penNum);
    void     penColourChanged(int penNum, QColor newColour);
    void     penThicknessChanged(int penNum, int newThickness);
    void     undoPenAction(int penNum);
    void     undoPrivateLines(void);
    void     redoPenAction(int penNum);
    bool     brushFromLast(int penNum);

    void     screenSnap(void);
    void     localScreenSnap(void);

    void     playbackDiscussion(void);

    void     updateClientsOnSessionState(void);

    void     recordDiscussion(void);
    void     recordReceivedPenRole(int receivedRole, int penNum);
    void     recordRemoteSessionType(int receivedType);
    void     sessionTypePrivate(int requestingPen);
    void     sessionTypeOverlay(int requestingPen);
    void     sessionTypeWhiteboard(int requestingPen);
    void     sessionTypeStickyNotes(int requestingPen);
    void     deleteStickyMode(int penNum);
    void     startmoveofStickyNote(int penNum);

private:
    QAction 	    *localMouseAction;

    QAction 	    *rate250,  *rate500,  *rate750,  *rate1000;
    QAction 	    *rate1500, *rate2000, *rate4000, *rate6000;

    QAction         *lastAction;

    QAction 	    *helpAction;
    QAction 	    *quitAction;

    bool             trayIconPresent;
    QSystemTrayIcon *trayIcon;
    QMenu           *trayIconMenu; // TODO: replace the following and build menu dynamically

    QMutex           readPenDataMutex;

    QColor           localPenColour;
};

#endif // OVERLAYWINDOW_H
