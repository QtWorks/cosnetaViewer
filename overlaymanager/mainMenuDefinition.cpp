// This has no header as it is intended to be included in another C++ file.

// Menus definition
#include "penStates.h"

//Set Menu Levels
//--------------------------
menuSet_t topLevel;
    menuSet_t penOptions;
        menuSet_t penType;
        menuSet_t penColour;
        menuSet_t penWidth;
        menuSet_t shapes;
        menuSet_t stickies;
        menuSet_t stickyColour;
    menuSet_t editOptions;
    menuSet_t pageOptions;
        menuSet_t pageColour;
    menuSet_t capture;
    menuSet_t settings;
    menuSet_t session;

//----------------------------

QColor colourRed         = Qt::red;
QColor colourGreen       = Qt::green;
QColor colourBlue        = Qt::blue;
QColor colourMagenta     = Qt::magenta;
QColor colourYellow      = Qt::yellow;
QColor colourDarkYellow  = Qt::darkYellow;
QColor colourWhite       = Qt::white;
QColor colourBlack       = Qt::black;
QColor colourGray        = Qt::gray;
QColor colourDarkGray    = Qt::darkGray;
QColor colourTransparent = Qt::transparent;

// View mode - no entries

// Overlay mode
menuEntry_t penOptionsMenuEntry  = {":/images/top/pen.svg",           NULL, "Pen Options",
                                    SESSION_ALL|PAGE_TYPE_ALL,        CHILD_MENU, &penOptions,        NULL, NULL };
//menuEntry_t stickiesMenuEntry    = {":/images/stickies.svg",          NULL, "Stickies",
//                                    SESSION_ALL|PAGE_TYPE_STICKIES,   CHILD_MENU, &stickies,          NULL, NULL };
#ifdef Q_OS_ANDROID
menuEntry_t editMenuEntry        = {":/menus/images/undo_redo.svg",   NULL, "Undo Redo",
                                    SESSION_ALL|PAGE_TYPE_ALL     ,   CHILD_MENU, &editOptions,          NULL, NULL };
menuEntry_t keyboardEntry        = {":/images/tablet/keyboard.svg",   NULL, "Keyboard",
                                    SESSION_ALL|PAGE_TYPE_ALL     ,   TOGGLE_KEYBOARD_INPUT, NULL,          NULL, NULL };
#endif
menuEntry_t pageOptionsMenuEntry = {":/images/top/page.svg",          NULL, "Page Options",
                                    SESSION_CONTROLLER|PAGE_TYPE_OVERLAY|PAGE_TYPE_WHITEBOARD,
                                                                      CHILD_MENU, &pageOptions,       NULL, NULL };
menuEntry_t captureMenuEntry     = {":/images/top/capture.svg",      NULL, "Capture",
                                    SESSION_CONTROLLER|PAGE_TYPE_OVERLAY|PAGE_TYPE_WHITEBOARD,
                                                                      CHILD_MENU, &capture,           NULL, NULL };
menuEntry_t snapshotTop          = {":/capture/images/capture/snapshot.svg", NULL, "Snapshot",
                                    SESSION_CONTROLLER|PAGE_TYPE_STICKIES, STORAGE_ACTION, (void *)STORAGE_SNAPSHOT, NULL, NULL };
#ifndef Q_OS_ANDROID
menuEntry_t settingsMenuEntry    = {":/images/top/settings.svg",      NULL, "Settings",
                                    SESSION_CONTROLLER|PAGE_TYPE_ALL, CHILD_MENU, &settings,          NULL, NULL };
#endif
menuEntry_t sessionTypeMenuEntry = {":/settings/images/settings/sessions.svg", NULL, "Session Type",
                                    SESSION_CONTROLLER|PAGE_TYPE_ALL, CHILD_MENU, &session,           NULL, NULL };
menuEntry_t helpEntry            = {":/images/top/help.svg",          NULL, "Help",
                                    SESSION_ALL|PAGE_TYPE_ALL,        APPLICATION_HELP, NULL,         NULL, NULL };
#ifdef Q_OS_ANDROID
menuEntry_t disconnectEntry      = {":/settings/images/settings/quit.svg", NULL, "Disconnect",
                                    SESSION_ALL|PAGE_TYPE_ALL, DISCONNECT_FROM_HOST, NULL,            NULL, NULL };
#endif


// Edit icons
menuEntry_t editUndo             = {":/images/top/undo.svg",          NULL, "Undo",
                                    SESSION_ALL|PAGE_TYPE_ALL,        EDIT_ACTION, (void *)EDIT_UNDO, NULL, NULL };
menuEntry_t editRedo             = {":/images/top/redo.svg",          NULL, "Redo",
                                    SESSION_ALL|PAGE_TYPE_ALL,        EDIT_ACTION, (void *)EDIT_REDO, NULL, NULL };
menuEntry_t doAgain              = {":/menus/images/do_again.svg",    NULL, "Do again",    SESSION_ALL|PAGE_TYPE_ALL, EDIT_ACTION, (void *)EDIT_DO_AGAIN,  NULL, NULL };


//Pen Option Icons
menuEntry_t penTypeMenuEntry     = {":/pen/images/pen/nib.svg",      NULL, "Pen Type",     SESSION_ALL|PAGE_TYPE_ALL, CHILD_MENU, &penType,   NULL, NULL };
menuEntry_t penColourMenuEntry   = {":/pen/images/pen/colour.svg",   NULL, "Pen Colour",   SESSION_ALL|PAGE_TYPE_ALL, CHILD_MENU, &penColour, NULL, NULL };
menuEntry_t penWidthMenuEntry    = {":/pen/images/pen/thickness.svg",NULL, "Pen Width",    SESSION_ALL|PAGE_TYPE_ALL, CHILD_MENU, &penWidth,  NULL, NULL };
menuEntry_t shapesMenuEntry      = {":/pen/images/pen/shapes.svg",   NULL, "Pen Shape",    SESSION_ALL|PAGE_TYPE_ALL, CHILD_MENU, &shapes,    NULL, NULL };


//Pen Type Icons
menuEntry_t penSimpleDraw    = {":/nib/images/nib/pen.svg",         NULL, "Draw",        SESSION_ALL|PAGE_TYPE_ALL, PEN_TYPE, (void *)DRAW_SOLID,  NULL, NULL };
menuEntry_t penDrawText      = {":/nib/images/nib/text.svg",        NULL, "Text",        SESSION_ALL|PAGE_TYPE_ALL, PEN_TYPE, (void *)DRAW_TEXT,   NULL, NULL };
menuEntry_t penHighlighter   = {":/nib/images/nib/highlighter.svg", NULL, "Highlighter", SESSION_ALL|PAGE_TYPE_ALL, PEN_TYPE, (void *)HIGHLIGHTER, NULL, NULL };
menuEntry_t penEraser        = {":/nib/images/nib/eraser.svg",      NULL, "Eraser",      SESSION_ALL|PAGE_TYPE_ALL, PEN_TYPE, (void *)ERASER,      NULL, NULL };
#if 0
menuEntry_t penSolid         = {"", NULL, "Solid",       SESSION_ALL|PAGE_TYPE_ALL, PEN_TYPE, (void *)DRAW_SOLID,      NULL, NULL };
menuEntry_t penDashed        = {"", NULL, "Dashed",      SESSION_ALL|PAGE_TYPE_ALL, PEN_TYPE, (void *)DRAW_DASHED,     NULL, NULL };
menuEntry_t penDotted        = {"", NULL, "Dotted",      SESSION_ALL|PAGE_TYPE_ALL, PEN_TYPE, (void *)DRAW_DOTTED,     NULL, NULL };
menuEntry_t penDotDashed     = {"", NULL, "Dot-Dashed",  SESSION_ALL|PAGE_TYPE_ALL, PEN_TYPE, (void *)DRAW_DOT_DASHED, NULL, NULL };
#endif


//Pen Colour Icons
menuEntry_t penRed          = {"", NULL, "Red",      SESSION_ALL|PAGE_TYPE_ALL, PEN_COLOUR_PRESET,   &colourRed,     NULL, NULL };
menuEntry_t penGreen        = {"", NULL, "Green",    SESSION_ALL|PAGE_TYPE_ALL, PEN_COLOUR_PRESET,   &colourGreen,   NULL, NULL };
menuEntry_t penBlue         = {"", NULL, "Blue",     SESSION_ALL|PAGE_TYPE_ALL, PEN_COLOUR_PRESET,   &colourBlue,    NULL, NULL };
menuEntry_t penMagenta      = {"", NULL, "Magenta",  SESSION_ALL|PAGE_TYPE_ALL, PEN_COLOUR_PRESET,   &colourMagenta, NULL, NULL };
menuEntry_t penYellow       = {"", NULL, "Yellow",   SESSION_ALL|PAGE_TYPE_ALL, PEN_COLOUR_PRESET,   &colourYellow,  NULL, NULL };
menuEntry_t penWhite        = {"", NULL, "White",    SESSION_ALL|PAGE_TYPE_ALL, PEN_COLOUR_PRESET,   &colourWhite,   NULL, NULL };
menuEntry_t penBlack        = {"", NULL, "Black",    SESSION_ALL|PAGE_TYPE_ALL, PEN_COLOUR_PRESET,   &colourBlack,   NULL, NULL };


//Pen Thickness Icons
menuEntry_t width2   = {":/width/images/width/2.svg",  NULL, "W:2",  SESSION_ALL|PAGE_TYPE_ALL, THICKNESS_PRESET, (void *)2,  NULL, NULL };
menuEntry_t width4   = {":/width/images/width/4.svg",  NULL, "W:4",  SESSION_ALL|PAGE_TYPE_ALL, THICKNESS_PRESET, (void *)4,  NULL, NULL };
menuEntry_t width8   = {":/width/images/width/8.svg",  NULL, "W:8",  SESSION_ALL|PAGE_TYPE_ALL, THICKNESS_PRESET, (void *)8,  NULL, NULL };
menuEntry_t width16  = {":/width/images/width/16.svg", NULL, "W:16", SESSION_ALL|PAGE_TYPE_ALL, THICKNESS_PRESET, (void *)16, NULL, NULL };
menuEntry_t width32  = {":/width/images/width/32.svg", NULL, "W:32", SESSION_ALL|PAGE_TYPE_ALL, THICKNESS_PRESET, (void *)32, NULL, NULL };
menuEntry_t width64  = {":/width/images/width/64.svg", NULL, "W:64", SESSION_ALL|PAGE_TYPE_ALL, THICKNESS_PRESET, (void *)64, NULL, NULL };


//Shapes Icons
menuEntry_t lineShape        = {":/shapes/images/shapes/line.svg",    NULL, "Line",     SESSION_ALL|PAGE_TYPE_ALL, PEN_SHAPE,   (void *)SHAPE_NONE    , NULL, NULL };
menuEntry_t triangleShape    = {":/shapes/images/shapes/triangle.svg",NULL, "Triangle", SESSION_ALL|PAGE_TYPE_ALL, PEN_SHAPE,   (void *)SHAPE_TRIANGLE, NULL, NULL };
menuEntry_t boxShape         = {":/shapes/images/shapes/square.svg",  NULL, "Square",   SESSION_ALL|PAGE_TYPE_ALL, PEN_SHAPE,   (void *)SHAPE_BOX,      NULL, NULL };
menuEntry_t circleShape      = {":/shapes/images/shapes/circle.svg",  NULL, "Circle",   SESSION_ALL|PAGE_TYPE_ALL, PEN_SHAPE,   (void *)SHAPE_CIRCLE,   NULL, NULL };
menuEntry_t stampLast        = {":/shapes/images/shapes/stamp.svg",   NULL, "Stamp",    SESSION_ALL|PAGE_TYPE_ALL, EDIT_ACTION, (void *)EDIT_DO_AGAIN,  NULL, NULL };


/*//Stikies Icons
menuEntry_t newSticky        = {":/stickies/images/stickies/new.svg",     NULL, "New",              SESSION_ALL|PAGE_TYPE_STICKIES,        STICKY_ACTION, (void *)STICKY_NEW,     NULL, NULL };
menuEntry_t moveSticky       = {":/stickies/images/stickies/move.svg",    NULL, "Move",             SESSION_ALL|PAGE_TYPE_STICKIES,        STICKY_ACTION, (void *)STICKY_MOVE,    NULL, NULL };
menuEntry_t stickyColourMenu = {":/page/images/page/colour.svg",          NULL, "Sticky Colour",    SESSION_ALL|PAGE_TYPE_STICKIES,        CHILD_MENU,    &stickyColour,          NULL, NULL };
menuEntry_t snapSticky       = {":/stickies/images/stickies/snap.svg",    NULL, "Snap to Grid",     SESSION_ALL|PAGE_TYPE_STICKIES,        STICKY_ACTION, (void *)STICKY_SNAP,    NULL, NULL };
menuEntry_t arrangeSticky    = {":/stickies/images/stickies/arrange.svg", NULL, "Arrange Stickies", SESSION_CONTROLLER|PAGE_TYPE_STICKIES, STICKY_ACTION, (void *)STICKY_ARRANGE, NULL, NULL };
menuEntry_t deleteSticky     = {":/stickies/images/stickies/delete.svg",  NULL, "Delete",           SESSION_ALL|PAGE_TYPE_STICKIES,        STICKY_ACTION, (void *)STICKY_DELETE,  NULL, NULL };
*/

//Page Options Icons
menuEntry_t newPage             = {":/page/images/page/new.svg",       NULL, "New Page",     SESSION_CONTROLLER|PAGE_TYPE_ALL, PAGE_ACTION,      (void *)PAGE_NEW,      NULL, NULL };
menuEntry_t nextPage            = {":/page/images/page/plus.svg",      NULL, "Next Page",    SESSION_CONTROLLER|PAGE_TYPE_ALL, PAGE_ACTION,      (void *)PAGE_NEXT,     NULL, NULL };
menuEntry_t prevPage            = {":/page/images/page/minus.svg",     NULL, "Prev Page",    SESSION_CONTROLLER|PAGE_TYPE_ALL, PAGE_ACTION,      (void *)PAGE_PREV,     NULL, NULL };
menuEntry_t selectPage          = {":/page/images/page/thumbnail.svg", NULL, "Select Page",  SESSION_CONTROLLER|PAGE_TYPE_ALL, PAGE_ACTION,      (void *)PAGE_SELECTOR, NULL, NULL };
menuEntry_t clearPage           = {":/page/images/page/clear.svg",     NULL, "Clear Page",   SESSION_CONTROLLER|PAGE_TYPE_ALL, PAGE_ACTION,      (void *)PAGE_CLEAR,    NULL, NULL };
menuEntry_t pageColourMenuEntry = {":/page/images/page/colour.svg",    NULL, "Paper Colour", SESSION_CONTROLLER|PAGE_TYPE_WHITEBOARD, CHILD_MENU, &pageColour,          NULL, NULL };


//Page Colour Icons
//menuEntry_t paperTransparent  = {"", NULL, "Transparent", SESSION_CONTROLLER|PAGE_TYPE_ALL, BACKGROUND_COLOUR_PRESET,   &colourTransparent, NULL, NULL };
menuEntry_t paperBlack        = {"", NULL, "Black",       SESSION_CONTROLLER|PAGE_TYPE_ALL, BACKGROUND_COLOUR_PRESET,   &colourBlack,      NULL, NULL };
menuEntry_t paperGreen        = {"", NULL, "Green",       SESSION_CONTROLLER|PAGE_TYPE_ALL, BACKGROUND_COLOUR_PRESET,   &colourGreen,      NULL, NULL };
menuEntry_t paperDarkGray     = {"", NULL, "Dark Gray",   SESSION_CONTROLLER|PAGE_TYPE_ALL, BACKGROUND_COLOUR_PRESET,   &colourDarkGray,   NULL, NULL };
menuEntry_t paperGray         = {"", NULL, "Gray",        SESSION_CONTROLLER|PAGE_TYPE_ALL, BACKGROUND_COLOUR_PRESET,   &colourGray,       NULL, NULL };
menuEntry_t paperWhite        = {"", NULL, "White",       SESSION_CONTROLLER|PAGE_TYPE_ALL, BACKGROUND_COLOUR_PRESET,   &colourWhite,      NULL, NULL };
menuEntry_t paperYellow       = {"", NULL, "Dark Yellow", SESSION_CONTROLLER|PAGE_TYPE_ALL, BACKGROUND_COLOUR_PRESET,   &colourYellow,     NULL, NULL };
menuEntry_t paperDarkYellow   = {"", NULL, "Dark Yellow", SESSION_CONTROLLER|PAGE_TYPE_ALL, BACKGROUND_COLOUR_PRESET,   &colourDarkYellow, NULL, NULL };
menuEntry_t paperBlue         = {"", NULL, "Blue",        SESSION_CONTROLLER|PAGE_TYPE_ALL, BACKGROUND_COLOUR_PRESET,   &colourBlue,       NULL, NULL };


//Capture Icons
menuEntry_t record    = {":/capture/images/capture/record.svg",   NULL, "Record Session",   SESSION_CONTROLLER|PAGE_TYPE_ALL, STORAGE_ACTION,   (void *)RECORD_SESSION,            NULL, NULL };
menuEntry_t playback  = {":/capture/images/capture/play.svg",     NULL, "Playback Session", SESSION_CONTROLLER|PAGE_TYPE_ALL, STORAGE_ACTION,   (void *)PLAYBACK_RECORDED_SESSION, NULL, NULL };
menuEntry_t snapshot  = {":/capture/images/capture/snapshot.svg", NULL, "Snapshot",         SESSION_CONTROLLER|PAGE_TYPE_ALL, STORAGE_ACTION,   (void *)STORAGE_SNAPSHOT,          NULL, NULL };
menuEntry_t gallery   = {":/capture/images/capture/gallery.svg",  NULL, "Gallery",          SESSION_CONTROLLER|PAGE_TYPE_ALL, STORAGE_ACTION,   (void *)SNAPSHOT_GALLERY,          NULL, NULL };
menuEntry_t saveAs    = {":/capture/images/capture/save.svg",     NULL, "Save As",          SESSION_CONTROLLER|PAGE_TYPE_ALL, STORAGE_ACTION,   (void *)STORAGE_SAVE,              NULL, NULL };


//Settings Icons
#ifndef Q_OS_ANDROID
menuEntry_t penManagement   = {":/settings/images/settings/control.svg", NULL, "Control Pen Access",   SESSION_CONTROLLER|PAGE_TYPE_ALL, HOST_OPTIONS,     (void *)CONTROL_PEN_ACCESS, NULL, NULL };
menuEntry_t hostedMode      = {":/settings/images/settings/host.svg",    NULL, "Hosted <-> Open",      SESSION_CONTROLLER|PAGE_TYPE_ALL, HOST_OPTIONS,     (void *)HOSTED_MODE_TOGGLE, NULL, NULL };
menuEntry_t mouseAsPen      = {":/settings/images/settings/mouse.svg",   NULL, "Local mouse is pen",   SESSION_CONTROLLER|PAGE_TYPE_ALL, HOST_OPTIONS,     (void *)MOUSE_AS_LOCAL_PEN, NULL, NULL };
menuEntry_t switchDisplay   = {":/settings/images/settings/screen.svg",  NULL, "Switch Screen",        SESSION_CONTROLLER|PAGE_TYPE_ALL, HOST_OPTIONS,     (void *)SWITCH_SCREEN,      NULL, NULL };
menuEntry_t quitApp         = {":/settings/images/settings/quit.svg",    NULL, "Quit",                 SESSION_CONTROLLER|PAGE_TYPE_ALL, QUIT_APPLICATION, NULL,                       NULL, NULL };
menuEntry_t toggleButtonStrip = {":/settings/images/settings/button_strip.svg", NULL, "Toggle buttons",       SESSION_CONTROLLER|PAGE_TYPE_ALL, TOGGLE_BUTTON_STRIP, NULL, NULL, NULL };
menuEntry_t toggleMenuType    = {":/settings/images/settings/menu.svg",         NULL, "Menu Type",            SESSION_ALL|PAGE_TYPE_ALL,        NEXT_MENU_TYPE,      NULL, NULL, NULL };
menuEntry_t allowRemoteView = {":/settings/images/settings/remote.svg",  NULL, "Allow Remote Connect", SESSION_CONTROLLER|PAGE_TYPE_ALL, HOST_OPTIONS,     (void *)ALLOW_REMOTE, NULL, NULL };
#endif


//Session type options
menuEntry_t sessionOverlay    = {":/images/top/overlay.svg",               NULL, "Annotaions",     SESSION_CONTROLLER|PAGE_TYPE_ALL, HOST_OPTIONS, (void *)SESSION_TYPE_OVERLAY,    NULL, NULL };
//menuEntry_t sessionWhiteboard = {":/page/images/page/whiteboard.svg",      NULL, "Whiteboard",     SESSION_CONTROLLER|PAGE_TYPE_ALL, HOST_OPTIONS, (void *)SESSION_TYPE_WHITEBOARD, NULL, NULL };
//menuEntry_t sessionStickies   = {":/images/stickies.svg",                  NULL, "Sticky Notes",   SESSION_CONTROLLER|PAGE_TYPE_ALL, HOST_OPTIONS, (void *)SESSION_TYPE_STICKIES,   NULL, NULL };
menuEntry_t remoteSession     = {":/settings/images/settings/remote.svg",  NULL, "Remote Session", SESSION_CONTROLLER|PAGE_TYPE_ALL, HOST_OPTIONS, (void *)REMOTE_SESSION,          NULL, NULL };

// Generate pen settings structure

void createPenMenuData(void)
{
    //Top Level Menu
    topLevel.entry.append(penOptionsMenuEntry);
#ifdef Q_OS_ANDROID
    topLevel.entry.append(editMenuEntry);
    topLevel.entry.append(clearPage);
#else
    topLevel.entry.append(editUndo);
    topLevel.entry.append(editRedo);
#endif
    //topLevel.entry.append(stickiesMenuEntry);
    topLevel.entry.append(pageOptionsMenuEntry);
    topLevel.entry.append(captureMenuEntry);
    topLevel.entry.append(snapshotTop);
    topLevel.entry.append(sessionTypeMenuEntry);
#ifndef Q_OS_ANDROID
    topLevel.entry.append(settingsMenuEntry);
#endif
    topLevel.entry.append(helpEntry);
#ifdef Q_OS_ANDROID
    topLevel.entry.append(disconnectEntry);
#endif
    topLevel.circleSizeForIconRender = 0;
    topLevel.parent = NULL;

    // Overlay controls
    //Pen Options Menu
    penOptions.entry.append(penColourMenuEntry);
    penOptions.entry.append(penWidthMenuEntry);
    penOptions.entry.append(penTypeMenuEntry);
    penOptions.entry.append(shapesMenuEntry);
    penOptions.circleSizeForIconRender = 0;
    penOptions.parent = &topLevel;

    //Pen Type Menu
    penType.entry.append(penSimpleDraw);
    //penType.entry.append(penDrawText);
#ifdef Q_OS_ANDROID
    penType.entry.append(keyboardEntry);
#endif
    penType.entry.append(penHighlighter);
    penType.entry.append(penEraser);
    penType.circleSizeForIconRender = 0;
    penType.parent = &penOptions;

#if 0
    penType.entry.append(penSolid);
    penType.entry.append(penDashed);
    penType.entry.append(penDotted);
    penType.entry.append(penDotDashed);
#endif

    //Pen Colour Menu
    penColour.entry.append(penRed);
    penColour.entry.append(penGreen);
    penColour.entry.append(penBlue);
    penColour.entry.append(penMagenta);
    penColour.entry.append(penYellow);
    penColour.entry.append(penWhite);
    penColour.entry.append(penBlack);
    penColour.circleSizeForIconRender = 0;
    penColour.parent = &penOptions;

    //Pen Thickness Menu
    penWidth.entry.append(width2);
    penWidth.entry.append(width4);
    penWidth.entry.append(width8);
    penWidth.entry.append(width16);
    penWidth.entry.append(width32);
    penWidth.entry.append(width64);
    penWidth.circleSizeForIconRender = 0;
    penWidth.parent = &penOptions;

    //Shapes Menu
    shapes.entry.append(triangleShape);
    shapes.entry.append(boxShape);
    shapes.entry.append(circleShape);
    shapes.entry.append(stampLast);
    shapes.entry.append(lineShape);
    shapes.circleSizeForIconRender = 0;
    shapes.parent = &penOptions;

    // Edit menu
#ifdef Q_OS_ANDROID
    editOptions.entry.append(editUndo);
    editOptions.entry.append(editRedo);
    editOptions.entry.append(doAgain);
    editOptions.circleSizeForIconRender = 0;
    editOptions.parent = &topLevel;
#endif

    //Stikies Menu
    //stickies.entry.append(newSticky);
    //stickies.entry.append(moveSticky);
    //stickies.entry.append(stickyColourMenu);
    //stickies.entry.append(snapSticky);
    //stickies.entry.append(arrangeSticky);
    //stickies.entry.append(deleteSticky);
    stickies.circleSizeForIconRender = 0;
    stickies.parent = &topLevel;

    //Sticky Colour Menu
    stickyColour.entry.append(paperDarkGray);
    stickyColour.entry.append(paperGray);
    stickyColour.entry.append(paperWhite);
    stickyColour.entry.append(paperYellow);
    stickyColour.entry.append(paperDarkYellow);
    stickyColour.entry.append(paperBlue);
    stickyColour.circleSizeForIconRender = 0;
    stickyColour.parent = &stickies;

    //Page Options Menu
    pageOptions.entry.append(pageColourMenuEntry);
    pageOptions.entry.append(newPage);
    pageOptions.entry.append(prevPage);
    pageOptions.entry.append(nextPage);
    pageOptions.entry.append(selectPage);
    //pageOptions.entry.append(clearPage);
    pageOptions.circleSizeForIconRender = 0;
    pageOptions.parent = &topLevel;

    //Paper Colour Menu
    //pageColour.entry.append(paperTransparent);
    pageColour.entry.append(paperBlack);
    pageColour.entry.append(paperDarkGray);
    pageColour.entry.append(paperGray);
    pageColour.entry.append(paperWhite);
    pageColour.entry.append(paperDarkYellow);
    pageColour.entry.append(paperBlue);
    pageColour.circleSizeForIconRender = 0;
    pageColour.parent = &pageOptions;

    //Capture Menu
    capture.entry.append(record);
    capture.entry.append(playback);
    capture.entry.append(snapshot);
    //capture.entry.append(gallery);
    //capture.entry.append(saveAs);
    capture.circleSizeForIconRender = 0;
    capture.parent = &topLevel;

    //Settings Menu
#ifndef Q_OS_ANDROID
    settings.entry.append(hostedMode);
//    settings.entry.append(penManagement);
    settings.entry.append(mouseAsPen);
//    settings.entry.append(switchDisplay);
//    settings.entry.append(allowRemoteView);
    settings.entry.append(toggleMenuType);
    settings.entry.append(toggleButtonStrip);
    settings.entry.append(quitApp);
    settings.circleSizeForIconRender = 0;
    settings.parent = &topLevel;
#endif
    //Session Menu
    session.entry.append(sessionOverlay);
    //session.entry.append(sessionWhiteboard);
    //session.entry.append(sessionStickies);
    session.entry.append(remoteSession);
    session.circleSizeForIconRender = 0;
    session.parent = &topLevel;
}

