#ifndef THREEBUTTONUI_H
#define THREEBUTTONUI_H

#include <QWidget>
#include <QHBoxLayout>
#include <QPushButton>
#include <QVBoxLayout>
#include <QLabel>
#include <QTimer>
#include <QColorDialog>
#include <QLabel>
#include <QString>

#include "colourslider.h"
#include "linethicknessselector.h"
#include "iconStrings.h"


class threeButtonUI : public QObject
{
    Q_OBJECT
    typedef enum
    {
        viewMode,
        privateMode,                              //pn
        overlayMode,
        presenterMode,
        mouseMode,
        stickiesMode,
        whiteboardMode,
        NODE_NOT_SET

    } current_menu_t;

    typedef enum
    {
        slide_left,
        slide_right,
        slide_up,
        slide_down

    } slide_icon_type_t;

    typedef enum
    {
        mode_buttons_out,
        mode_buttons_move_out,
        mode_buttons_move_in,
        mode_buttons_in
    } mode_buttons_state_type_t;

    typedef enum
    {
        pen_colour, whiteboard_colour, priv_board_colour, sticky_note_colour
    } colour_selector_mode_t;

    typedef enum
    {
        tabs_hidden,
        tab_sticky,
        tab_undo,
        tab_main_controls,
//        tab_shapes,
        tab_page_controls,
        tab_capture,
        tab_p_sticky,                                     //pn
        tab_p_undo,                                       //pn
        tab_p_main_controls,                              //pn
        tab_p_shapes,                                     //pn
        tab_p_page_controls,                              //pn
        tab_p_capture                                     //pn
    } overlay_tab_t; //private_tab_t; In case privacy and annotate need seperated

public:
    explicit threeButtonUI(int pseudoPenNum, bool highRes, QWidget *parent = 0);

    // Interface functions:
    void            screenResized(int width, int newHeight); // Let's us explicitly place UI elements based on screen size
    void            setSessionType(int newSessionType);
    void            setBusyMode(void);
    void            endBusyMode(void);
    void            setViewMode(void);
    void            setMenuMode(current_menu_t newMode);
    int             getMenuMode(void);
    void            makePeer(void);
    void            makeSessionLeader(void);
    void            makeFollower(void);
    void            displayMessage(QString message, int durationMS);
    void            raise(void);
    void            hide(void);
    void            show(void);
    bool            isVisible(void);
    QPoint          pos(void);
    QSize           size(void);

private:
    void         createModeSwitchButtonBar(void);
    void         createNotificationArea(void);
    void         createAnnotationButtonArea(void);
    void         createPrivateButtonArea(void);                                             //pn. Might not be needed
//    void         createStickyTabsMenu(void);

    void         createUndoTabsMenu(void);
    void         createMainControlsTabsMenu(void);
    void         createPenShapesTabsMenu(void);
    void         createPageControlsTabsMenu(void);
    void         createCaptureTabsMenu(void);

    void         createPrivateUndoTabsMenu(void);                                           //pn
    void         createPrivateMainControlsTabsMenu(void);                                   //pn
    void         createPrivatePenShapesTabsMenu(void);                                      //pn
    void         createPrivatePageControlsTabsMenu(void);                                   //pn
    void         createPrivateCaptureTabsMenu(void);                                        //pn

    QHBoxLayout *createTopTabButtonArea(void);
    QHBoxLayout *createPrivateTopTabButtonArea(void);                                       //pn

    void         currentTabSwitch(overlay_tab_t changedTab);
    //void         currentTabSwitch(private_tab_t changedTab);                              //pn

    QHBoxLayout *createStickyButtonArea(void);
    QHBoxLayout *createMainMenuUndoRedoRow(void);
    QVBoxLayout *createMainMenuControlsRow(void);
    QHBoxLayout *createMainMenuPenShapeRow(void);
    QVBoxLayout *createMainMenuPageControlsRow(void);
    QHBoxLayout *createMainMenuCaptureControlsRow(void);

    QHBoxLayout *createPrivateStickyButtonArea(void);                                           //pn
    QHBoxLayout *createPrivateMainMenuUndoRedoRow(void);                                        //pn
    QVBoxLayout *createPrivateMainMenuControlsRow(void);                                        //pn
    QHBoxLayout *createPrivateMainMenuPenShapeRow(void);                                        //pn
    QVBoxLayout *createPrivateMainMenuPageControlsRow(void);                                    //pn
    QHBoxLayout *createPrivateMainMenuCaptureControlsRow(void);                                 //pn

    QHBoxLayout *penTypeText(void);
    QHBoxLayout *penShapeText(void);
    QHBoxLayout *undoRedoText(void);
    QHBoxLayout *pageBackgroundText(void);
    QHBoxLayout *changePageNumText(void);
    QHBoxLayout *saveSnapshotText(void);
    QHBoxLayout *printTextHeaders(QString displayText);

    void         createPresentorButtonArea(void);
    void         createMouseButtonArea(void);
    void         createThreeButtonArea(void);

    void     setIconFromResource(QPushButton *button, QString res, int iconWidth, int iconHeight);
    void     setIconFromResource(QPushButton *button, QString res, int iconWidth, int iconHeight, slide_icon_type_t arrowType);
    void     buttonModeAnimator(void);
    void     sessionStateChangeAnimate(void);
    void     sessionStateChangeComplete(void);
    void     setExpandModeButtonsIconCurrentImage();
    void     getColourSelector(void);


private slots:

    void     modesClicked(bool checked);
    void     logoClicked(bool checked);
    void     expandMenuClicked(bool checked);

    void     quitClicked(bool checked);
    void     viewModeClicked(bool checked);
    void     privateModeClicked(bool checked);                                      //pn
    void     annotateModeClicked(bool checked);
    void     whiteboardModeClicked(bool checked);
    void     stickyModeClicked(bool checked);
    void     presentationModeClicked(bool checked);
    void     mouseModeClicked(bool checked);

    void     stickyClicked(bool checked);
    void     undoClicked(bool checked);
//    void     colourClicked(bool checked);
    void     linesClicked(bool checked);
    void     pageClicked(bool checked);
    void     snapshotClicked(bool checked);

    void     privateLinesClicked(bool checked);                                    //pn
    void     privateUndoClicked(bool checked);                                     //pn
    void     privatePageClicked(bool checked);                                     //pn
    void     privateSnapshotClicked(bool checked);                                 //pn

    void     hideNotification();

    void     dummyMenuModeCall(bool checked);

    void     localScreenSnap(bool checked);
    void     remoteScreenSnap(bool checked);

    void     nextSlideClicked(bool checked);
    void     prevSlideClicked(bool checked);
    void     toggleLaserClicked(bool checked);
    void     startPresentationClicked(bool checked);
    void     stopPresentationClicked(bool checked);
    void     defaultPresentationClicked(bool checked);

    void     middleMouseClicked(bool checked);
    void     rightMouseClicked(bool checked);
    void     scrollUpMouseClicked(bool checked);
    void     scrollDownClicked(bool checked);
    void     sendTextClicked(bool checked);

    void     chooseNewPenColour(bool checked);
    void     chooseNewSliderColour(bool checked);   //Colour Slider
    void     chooseNewPenThickness(bool checked);
    void     chooseNewPaperColour(bool checked);
    void     transparentOverlay(bool checked);
    void     toggleTextInput(bool checked);

    void     chooseNewPPaperColour(bool checked);                                       //pn
    void     transparentPrivateOverlay(bool checked);                                   //pn
    void     togglePrivateTextInput(bool checked);                                      //pn

    void     undoPenAction(bool checked);
    void     redoPenAction(bool checked);
    void     brushFromLast(bool checked);

    void     undoPrivatePenAction(bool checked);                                        //pn
    void     redoPrivatePenAction(bool checked);                                        //pn
    //void     privateBrushFromLast(bool checked);                                        //pn


    void     newPage(bool checked);
    void     nextPage(bool checked);
    void     prevPage(bool checked);
    void     showPageSelector(bool checked);
    void     clearOverlay(bool checked);

    void     clearPOverlay(bool checked);                                             //pn

    void     addNewStickyStart(bool checked);
    void     deleteStickyMode(bool checked);
    void     startmoveofStickyNote(bool checked);
    void     setStickyNotePaperColour(bool checked);

    //void     addNewPrivateStickyStart(bool checked);                                //pn
    //void     deletePrivateStickyMode(bool checked);                                 //pn
    //void     startmoveofPrivateStickyMode(bool checked);                            //pn
    //void     setPrivateStickyNotePaperColour(bool checked);                         //pn

    void     hostedModeToggle(bool checked);
    void     recordDiscussion(bool checked);
    void     playbackDiscussion(bool checked);

    void     drawTypeLine(bool checked);
    void     drawTypeTriangle(bool checked);
    void     drawTypeBox(bool checked);
    void     drawTypeCircle(bool checked);
    void     drawTypeStamp(bool checked);

    void     privateDrawTypeLine(bool checked);                                     //pn
    void     privateDrawTypeTriangle(bool checked);                                 //pn
    void     privateDrawTypeBox(bool checked);                                      //pn
    void     privateDrawTypeCircle(bool checked);                                   //pn
    void     privateDrawTypeStamp(bool checked);                                    //pn

    //Qt's Colour Widget
    void     colourSelected(QColor &newColour);
    void     colourAccepted();
    void     colourSelectCancelled();

    //Colour Sliders
    void     newSliderValue(int redSliderValue, int greenSliderValue, int blueSliderValue);
    void     cancelSliderColourSel();

    //Thickness Slider
    void     newThickness(int thickness);
    void     cancelledThiocknesSel();


signals:

public slots:

private:
    // UI elements, with which to tinker
    QWidget     *threeButtonBar;
    QPushButton *expandModesButton;
    QPushButton *logoButton;
    QPushButton *logoPanZoomButton;
    QPushButton *expandMenuButton;
    QPushButton *expandPrivateMenuButton;
    QPushButton *screenSnapButton;

    QWidget     *modeButtonsBar;
    QPushButton *quitModeButton;
    QPushButton *viewModeButton;
    QPushButton *privateModeButton;                                     //pn
    QPushButton *annotateModeButton;
//    QPushButton *whiteboardModeButton;
//    QPushButton *stickyModeButton;
    QPushButton *presentationModeButton;
    QPushButton *mouseModeButton;

    QWidget     *menuButtonsArea;
//    QWidget     *stickyTabButtonsArea;
    QWidget     *undoTabButtonsArea;
    QWidget     *mainControlsTabButtonsArea;
//    QWidget     *penShapesTabButtonsArea;
    QWidget     *pageControlsTabButtonsArea;
    QWidget     *captureTabButtonsArea;
    QHBoxLayout *stickyNotesArea;

    QWidget     *privateMenuButtonsArea;                                            //pn
    //QWidget     *privateStickyTabButtonsArea;                                       //pn
    QWidget     *privateUndoTabButtonsArea;                                         //pn
    QWidget     *privateMainControlsTabButtonsArea;                                 //pn
    //QWidget     *privatePenShapesTabButtonsArea;                                    //pn
    QWidget     *privatePageControlsTabButtonsArea;                                 //pn
    QWidget     *privateCaptureTabButtonsArea;                                      //pn
    QWidget     *privateStickyNotesArea;                                            //pn

    QPushButton *penTabs;
    QPushButton *undoTabs;
    QPushButton *clearPageTabs;
    QPushButton *newStickyTabs;
    QPushButton *snapshotTabs;

    QPushButton *privatePenTabs;                                                //pn
    QPushButton *privateUndoTabs;                                               //pn
    QPushButton *privateClearPageTabs;                                          //pn
    QPushButton *privateSnapshotTabs;                                           //pn

    int          rowsDisplayed;

    QWidget     *presenterButtonsBar;
    QPushButton *nextSlideButton;
    QPushButton *prevSlideButton;
    QPushButton *toggleLaserButton;
    QPushButton *startPresentationButton;
    QPushButton *stopPresentationButton;
    QPushButton *defaultPresentationButton;

    QWidget     *mouseButtonsBar;
    QPushButton *middleMouseButton;
    QPushButton *rightMouseButton;
    QPushButton *scrollUpMouseButton;
    QPushButton *scrollDownMouseButton;
    QPushButton *sendTextButton;

    QWidget     *notificationArea;
    QLabel      *notificationText;
    QTimer      *hideNotificationTimer;

    QColorDialog          *colourSelector;
    colour_selector_mode_t colourSelectMode;

    colourSlider          *sliderColourSel;
    lineThicknessSelector *thicknessSel;

    // Data
    iconStrings                icons;
    int                        menuBuffer;
    int                        xMultiplier;
    bool                       uiLockedUntilServerActionComplete; // rename
    bool                       highResDisplay;
    int                        penNum;
    QWidget                   *parentWidget;
    int                        parentWidth, parentHeight;
    current_menu_t             currentMode;
    //private_tab_t              currentTab;                                    //pn
    overlay_tab_t              currentTab;
    int                        buttonWidth,buttonHeight;
    int                        threeButtonY, menuAreaY;
    bool                       keyboardAvailable;
    bool                       busyMode;
    QString                    newIconResourceString;
    int                        redSliderEntry;
    int                        greenSliderEntry;
    int                        blueSliderEntry;
    int                        lineThickness;

    // Session data
    bool                       remoteEditMode;
    int                        currentSessionType;
    QColor                     remoteSessionColour;

    // Menu mode/state
    mode_buttons_state_type_t  modeButtonsState;
    int                        modeButtonsPercentOut;
    bool                       modeButtonAnimatorRunning;

    mode_buttons_state_type_t  mouseButtonsState;
    int                        mouseButtonsPercentOut;

};

#endif // THREEBUTTONUI_H
