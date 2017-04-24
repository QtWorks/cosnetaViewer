#ifndef PENSTATES_H
#define PENSTATES_H

#include <QString>
#include <QImage>
#include <QList>

// Type data for defining states

#define SESSION_LEADER     (1<<0)
#define SESSION_FOLLOWER   (1<<1)
#define SESSION_PEER       (1<<2)
#define SESSION_ALL        (SESSION_LEADER|SESSION_FOLLOWER|SESSION_PEER)
#define SESSION_CONTROLLER (SESSION_LEADER|SESSION_PEER)

#define PAGE_TYPE_PRIVATE    (1<<7)
#define PAGE_TYPE_OVERLAY    (1<<4)
#define PAGE_TYPE_STICKIES   (1<<5)
#define PAGE_TYPE_WHITEBOARD (1<<6)
#define PAGE_TYPE_ALL        (PAGE_TYPE_OVERLAY|PAGE_TYPE_STICKIES|PAGE_TYPE_WHITEBOARD)

typedef int penRole_t;

typedef enum
{
    DRAW_SOLID=1, DRAW_DASHED, DRAW_DOTTED, DRAW_DOT_DASHED,
    DRAW_TEXT,
    ERASER,
    HIGHLIGHTER,
    EDIT

} penAction_t;


typedef enum
{
    NO_EDIT=1,
    EDIT_UNDO,
    EDIT_REDO,
    EDIT_DO_AGAIN

} pen_edit_t;

typedef enum
{
    STORAGE_SNAPSHOT=1,
    RECORD_SESSION,
    PLAYBACK_RECORDED_SESSION,
    SNAPSHOT_GALLERY,
    STORAGE_SAVE

} storageAction_t;

typedef enum
{
    MOUSE_LEFT=1,
    MOUSE_MIDDLE,
    MOUSE_RIGHT,
    MOUSE_SCROLLUP,
    MOUSE_SCROLLDOWN

} fakeMouseButtonAction_t;

typedef enum
{
    PAGE_NEW=1, PAGE_NEXT, PAGE_PREV, PAGE_SELECTOR, PAGE_CLEAR

} pageAction_t;

typedef enum
{
    STICKY_NEW=1, STICKY_MOVE, STICKY_DELETE, STICKY_SNAP, STICKY_ARRANGE, STICKY_ANIMATE

} stickyAction_t;

typedef enum
{
    SHAPE_NONE=1, SHAPE_TRIANGLE, SHAPE_BOX, SHAPE_CIRCLE

} shape_t;

typedef enum
{
    CONTROL_PEN_ACCESS=1, HOSTED_MODE_TOGGLE,      SWITCH_SCREEN,
    MOUSE_AS_LOCAL_PEN,   REMOTE_SESSION,          ALLOW_REMOTE,
    SESSION_TYPE_PRIVATE, SESSION_TYPE_OVERLAY, SESSION_TYPE_WHITEBOARD, SESSION_TYPE_STICKIES

} hostOptions_t;

typedef enum
{
    NEXT_SLIDE, PREV_SLIDE, SHOW_HIGHLIGHT, START_SHOW, STOP_SHOW, START_DEFAULT

} presentation_controls_t;

// Menu tree data encapsulating types

typedef enum
{
    // Command class (these are for data that uses the three other bytes)
    PEN_COLOUR_PRESET        = 0x10000,
    BACKGROUND_COLOUR_PRESET = 0x20000,
    BACKGROUND_TRANSPARENT   = 0x30000,
    TOP_BYTE_TAG_COMMAND     = 0xF0000, // Use the normal type below, placed in top byte.

    CHILD_MENU = 1,
    PEN_COLOUR_SELECTOR,
    BACKGROUND_COLOUR_SELECTOR,
    THICKNESS_PRESET,         THICKNESS_SLIDER,
    PEN_TYPE,                                       // Draw/HighLight/Erase
    PEN_SHAPE,                                      // Shape_Square/Shape_Circle
    EDIT_ACTION,                                    // Undo/Redo/Do-again
    PAGE_ACTION,                                    // New page, prev page, next page
    STORAGE_ACTION,                                 // Record/Playback/Snapshot/SaveSlideShow
    STICKY_ACTION,
    GENERIC_DIALOGUE, NEXT_MENU_TYPE, TOGGLE_BUTTON_STRIP,
    SET_PEN_NAME, SET_PEN_SENSITIVITY,
    HOST_OPTIONS, PEN_CONFIGURATION, APPLICATION_HELP,
    INPUT_IS_VIEW_CONTROL, INPUT_IS_PRIVATE, INPUT_IS_OVERLAY_DRAW,
    INPUT_IS_PRESENTATION_CTRL, INPUT_IS_MOUSE,
    TOGGLE_KEYBOARD_INPUT, /* 24 */
    LEFT_MOUSE_TOGGLE, MIDDLE_MOUSE_PRESS, RIGHT_MOUSE_PRESS,
    MOUSE_SCROLLDOWN_PRESS, MOUSE_SCROLLUP_PRESS,
    PRESENTATION_CONTROL,
    DISCONNECT_FROM_HOST,
    QUIT_APPLICATION,
    TOGGLE_MENU, MENU_PARENT

} menuEntryType_t;

#define MENU_ENTRY_TYPE_CLASS(x) ((x) & 0xF0000)
#define COMMAND_FROM_TOP_BYTE(x) (((x) >> 24) & 255)
#define COMMAND_TO_TOP_BYTE(x)   ((((x)&255) << 24)|TOP_BYTE_TAG_COMMAND)

typedef struct menuEntry_s
{
    QString             icon_resource; // Identifier of SVG so we can rescale the icon
    QImage             *icon;          // To display in menu
    QString             altText;       // Hover text (if required at a later date)
    penRole_t           validRoles;    // Visible to leader or follower

    menuEntryType_t     type;          // What this entry does when selected
    void               *data;          // Arbritrary data (can be a child menu too)
    void               *parentMenuset; // Parent (these 2 are filled in by buttonStrip code)
    void               *buttonWidget;  // Search here through widgets and link back

} menuEntry_t;

typedef struct menuSet_s
{
    struct menuSet_s    *parent;

    int                  circleSizeForIconRender;
    QVector<menuEntry_t> entry;

} menuSet_t;

// This allows access to the data stored in the menu data blob.
extern menuSet_t topLevel;


typedef enum
{
    NO_ACTION,
    CLICKED,
    RELEASED,
    DOUBLE_CLICKED, // Probably never to be used
    LONG_CLICK      // TBD - seems in conflict with double click etc

} buttonAction_t;

typedef struct
{
    buttonAction_t leftMouseAction;
    buttonAction_t centreMouseAction;
    buttonAction_t rightMouseAction;
    buttonAction_t scrollPlusAction;
    buttonAction_t scrollMinusAction;
    buttonAction_t modeToggleAction;
    buttonAction_t precisionToggleAction;
    buttonAction_t gestureToggleAction;

    bool           leftMouse;
    bool           centreMouse;
    bool           rightMouse;
    bool           scrollPlus;
    bool           scrollMinus;
    bool           modeToggle;
    bool           precisionToggle;
    bool           gestureToggle;

} buttonsActions_t;

#define GENERIC_ZOOM_IN(buttonActions) \
    ( (buttonActions).centreMouse && \
      (buttonActions).scrollPlusAction == CLICKED )
#define GENERIC_ZOOM_OUT(buttonActions) \
    ( (buttonActions).centreMouse && \
      (buttonActions).scrollMinusAction == CLICKED )

#endif // PENSTATES_H
