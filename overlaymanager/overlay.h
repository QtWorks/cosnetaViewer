#ifndef OVERLAY_H
#define OVERLAY_H

#include <QObject>
#include <QString>
#include <QImage>
#include <QColor>
#include <QFile>
#include <QDataStream>
#include <QTime>
#include <QPixmap>
#include <QByteArray>
#include <QDataStream>
#include <QPainter>
#include <QTcpServer>
#include <QList>
#include <QVector>

#include <vector>

#include "build_options.h"

#include "penCursor.h"
#include "viewServer.h"
#include "penStates.h"
#include "textOverlay.h"
#include "viewServerListener.h"
#include "cosnetaAPI.h"

#define PI 3.141592653589793

//#define DEFAULT_HOST_IP               "example.cosneta.com"  /* Restore when not working on android */
#define DEFAULT_HOST_IP               "192.168.2.63"   /* as every download looses last defaults */
#define DEFAULT_HOST_CONTROL_PORT      14753           /* sysTray port number   */
#define DEFAULT_HOST_VIEW_PORT         14754           /* freeStyle port number */
#define DEFAULT_HOST_CONTROL_PORT_STR "14753"          /* sysTray port number   */
#define DEFAULT_HOST_VIEW_PORT_STR    "14754"          /* freeStyle port number */

#define APPLICATION_SETTINGS_FILENAME "freeStyleQt.ini"
#define SETS_HOSTNAME     "last_remote/hostName"
#define SETS_HOSTPORT     "last_remote/hostPort"
#define SETS_HAS_HOSTVIEW "last_remote/differentViewer"
#define SETS_HOSTVIEWNAME "last_remote/hostViewRepeaterName"
#define SETS_HOSTVIEWPORT "last_remote/hostViewRepeaterPort"

// Allow for more pens than available through the cosnetaAPI. At present,
// we use this to allow the mouse to be a pen too.
#define LOCAL_MOUSE_PEN (MAX_PENS)
#define MAX_SCREEN_PENS ((MAX_PENS)+1)

typedef enum { TYPE_DRAW=1,        TYPE_HIGHLIGHTER, TYPE_ERASER      } pen_draw_style_t;
typedef enum { ACTION_TYPE_NONE=1, ACTION_TYPE_DRAW, ACTION_TYPE_TEXT } pen_action_t;

#define PEN_TYPE_QSTR(t) ((t)==TYPE_DRAW?QString("TYPE_DRAW"):                   \
                          (t)==TYPE_HIGHLIGHTER?QString("TYPE_HIGHLIGHTER"):     \
                          (t)==TYPE_ERASER?QString("TYPE_ERASER"):               \
                                                  QString("TYPE_?????"))

typedef struct
{
    QPoint                    pos;
    int                       thickness;
    pen_draw_style_t          drawStyle;
    QColor                    colour;
    QColor                    applyColour;
    pen_action_t              actionType;
    bool                      wasDrawing;
    bool                      posPrevPresent;
    bool                      isTextPen;
    bool                      addingText;
    QPoint                    textPos;

} pens_state_t;

// undo points for a pen
typedef struct
{
    bool               active;
    QVector< int >     actionIndex; // Index in actionArray of the

} undoPenData_t;

typedef struct
{
    undoPenData_t         undo[MAX_SCREEN_PENS];
#ifdef SEND_ACTIONS_TO_CLIENTS
    QVector<quint32>      allActions;           // Same as previous actions except, includes "UNDO, REDO etc" to send
#endif
    QVector<quint32>      previousActions;
    pens_state_t          startPenState[MAX_SCREEN_PENS];
    pens_state_t          currentPens[MAX_SCREEN_PENS];    // Let's not trawl the actions...
    QColor                startBackgroundColour;
    QColor                currentBackgroundColour;

    // Text overlay (all at the same font size)
    QFont                *textLayerFont;

} pageData_t;


typedef enum { ZOOM_IN=1, ZOOM_OUT } zoom_t;



// Move to superclass before long...
enum ACTION_id_e
{
    ACTION_TIMEPOINT=1,
    ACTION_ACKNOWLEDGE, ACTION_MENU_OPT,
    ACTION_SCREENSIZE,
    ACTION_BACKGROUND_PNG, ACTION_BACKGROUND_JPG,
    ACTION_BACKGROUND_BA_COMPRESSED, ACTION_BG_JPEG_TILE,
    ACTION_BG_PNG_TILE,              ACTION_BACKGROUND_SAME_TILE,
    ACTION_BACKGROUND_COLOUR,
    ACTION_STARTDRAWING,   ACTION_STOPDRAWING,
    ACTION_PENPOSITIONS,
    ACTION_PENCOLOUR, ACTION_PENWIDTH /* 16 */, ACTION_PENTYPE, ACTION_TEXT,
    ACTION_UNDO, ACTION_REDO, ACTION_DOBRUSH, ACTION_BRUSHFROMLAST,
    ACTION_TRIANGLE_BRUSH, ACTION_BOX_BRUSH, ACTION_CIRCLE_BRUSH,
    ACTION_BRUSH_ZOOM,
    ACTION_NEXTPAGE, ACTION_PREVPAGE, ACTION_GOTOPAGE,
    ACTION_CLEAR_SCREEN,
    ACTION_SET_TEXT_CURSOR_POS, ACTION_APPLY_TEXT_CHAR /* 32 */,
    ACTION_CLIENT_PAGE_CHANGE,
    ACTION_ADD_STICKY_NOTE,  ACTION_DELETE_STICKY_NOTE,
    ACTION_MOVE_STICKY_NOTE, ACTION_ZOOM_STICKY_NOTE,
    ACTION_SESSION_STATE, ACTION_PEN_STATUS,
    ACTION_PROBE_VIEW_BROADCAST, ACTION_VIEW_BROADCAST_ACK, ACTION_VIEW_BROADCAST_CLIENT_STATUS,
    ACTION_PENS_PRESENT, ACTION_BACKGROUND_BA_COMPR_PLUS_REPEATS

};
// Possibly use this in undo/redo annotations (but the use tr())
#define ACTION_DEBUG(x)  ((x) == ACTION_TIMEPOINT         ? "TIMEPOINT" : \
                          (x) == ACTION_SCREENSIZE        ? "Screen size" : \
                          (x) == ACTION_ACKNOWLEDGE       ? "Acknowledge" : \
                          (x) == ACTION_MENU_OPT          ? "Menu option" : \
                          (x) == ACTION_BACKGROUND_PNG    ? "Background Image" : \
                          (x) == ACTION_BACKGROUND_COLOUR ? "Background Colour" : \
                          (x) == ACTION_STARTDRAWING      ? "Start Drawing" : \
                          (x) == ACTION_STOPDRAWING       ? "Stop Drawing" : \
                          (x) == ACTION_PENPOSITIONS      ? "Pen Positions" : \
                          (x) == ACTION_PENCOLOUR         ? "Pen Colour" : \
                          (x) == ACTION_PENWIDTH          ? "Pen Width" : \
                          (x) == ACTION_PENTYPE           ? "Pen Type" : \
                          (x) == ACTION_UNDO              ? "Undo" : \
                          (x) == ACTION_REDO              ? "Redo" : \
                          (x) == ACTION_DOBRUSH           ? "Do brush" : \
                          (x) == ACTION_BRUSHFROMLAST     ? "Brush from last" : \
                          (x) == ACTION_TRIANGLE_BRUSH    ? "Triangle brush" : \
                          (x) == ACTION_BOX_BRUSH         ? "Box brush" : \
                          (x) == ACTION_CIRCLE_BRUSH      ? "Circle brush" : \
                          (x) == ACTION_BRUSH_ZOOM        ? "Brush zoom" : \
                          (x) == ACTION_NEXTPAGE          ? "Next page" : \
                          (x) == ACTION_PREVPAGE          ? "Previous page" : \
                          (x) == ACTION_GOTOPAGE          ? "Goto page" : \
                          (x) == ACTION_BG_PNG_TILE       ? "Background update" : \
                          (x) == ACTION_BACKGROUND_SAME_TILE     ? "Repeat last tile" : \
                          (x) == ACTION_CLEAR_SCREEN             ? "Clear screen" : \
                          (x) == ACTION_SET_TEXT_CURSOR_POS      ? "Click new text cursor pos" : \
                          (x) == ACTION_APPLY_TEXT_CHAR          ? "Apply text character"      : \
                          (x) == ACTION_CLIENT_PAGE_CHANGE       ? "Client page change" : \
                          (x) == ACTION_ADD_STICKY_NOTE          ? "Add sticky note" :\
                          (x) == ACTION_DELETE_STICKY_NOTE       ? "Delete sticky note" : \
                          (x) == ACTION_MOVE_STICKY_NOTE         ? "Move sticky note" : \
                          (x) == ACTION_BACKGROUND_BA_COMPRESSED ? "compressed tile" : \
                          (x) == ACTION_BG_JPEG_TILE             ? "big JPG tile" : \
                          (x) == ACTION_BACKGROUND_SAME_TILE     ? "tile repeats" : \
                          (x) == ACTION_PROBE_VIEW_BROADCAST     ? "View broadcast PROBE" : \
                          (x) == ACTION_VIEW_BROADCAST_ACK       ? "View broadcast ACK" : \
                          (x) == ACTION_VIEW_BROADCAST_CLIENT_STATUS ? "View broadcast client status" : \
                          (x) == ACTION_ZOOM_STICKY_NOTE         ? "Zoom sticky note" : \
                          (x) == ACTION_PENS_PRESENT             ? "pens present list" : \
                                                                   "????" )
#define ACTION_DEBUG_WD(x) ACTION_DEBUG(((x)>>24)&0x7F)
#define ACTION_TYPE(x)     ((x)>>24)

// Play with these so we don't have too much overhead OR too much jitter (delay)
#define UPDATE_IMAGE_WIDTH  8
#define UPDATE_IMAGE_HEIGHT 8

#ifdef TABLET_MODE
// A bit of a nasty hack to minimise code changes when removing swathes of functionality.
#define screenGrabber void
#define ViewServer    void
#endif

class overlay : public QWidget
{
    Q_OBJECT
public:
    explicit overlay(QImage    *image,      QImage        *highlight, QImage *textrender,
                     QColor    *initColour, QColor         backgroundColour,
                     bool       canSend,    screenGrabber *grabTask,
                     QWidget   *parent = 0);
    ~overlay(void);

    void    setDefaults(void);

    void    setBackground(QColor bgColour);
    void    setScreenSize(int newW, int newH);
    QImage *imageData(void);

    void    rebuildAnnotationsFromUndoList(void);

    textOverlay *txtOverlay;
    
    bool    penIsPresent(int penNum);

    void    setPenColour(int penNum, QColor col);
    QColor  getPenColour(int penNum);
    void    setPenThickness(int penNum, int thick);
    int     getPenThickness(int penNum);

    void    updateClientsOnSessionState(int sessionType);
    void    updateClientsOnPenRole(int penNum, penRole_t newRole);

    void    startPenDraw(int penNum);
    void    stopPenDraw(int penNum);

    void    penTypeDraw(int penNum);
    void    penTypeText(int penNum);
    void    penTypeHighlighter(int penNum);
    void    penTypeEraser(int penNum);

    void    setPageInitialBackground(QColor initColour);
    void    ensureTextAreaIsBigEnough(int row, int col);
    void    applyReturn(int &row, int &col);
    void    wordWrap(int &row, int &col);
    void    setTextPosFromPixelPos(int penNum, int x, int y);
    void    addTextCharacterToOverlay(int penNum, quint32 charData);

    void    nextPage(void);
    void    previousPage(void);
    void    gotoPage(int number);
    void    sendPageChangeLk(void);

    void    addStickyNote(int size, QPoint pos, int penNum);
    void    deleteStickyNote(int noteNum, int penNum);
    void    moveStickyNote(QPoint newPos, int stickyNum, int penNum);
    void    zoomStickyNote(zoom_t zoom, int stickyNum, int penNum);

    void    clearOverlay(void);

    void    penPositions(bool penPresent[], int xLoc[], int yLoc[]);

    void    undoPenAction(int penNum);
    void    redoPenAction(int penNum);
    void    CurrentBrushAction(int penNum, int x, int y);
    bool    brushFromLast(int penNum);
    void    explicitBrush(int penNum, shape_t shape, int param1, int param2);
    void    zoomBrush(int penNum, int scalePercent);
    QList<QPoint> *getBrushPoints(int penNum);

    QString decodeWords(QVector<quint32> &words, int index);

    QColor  backgroundColour;

signals:
    
public slots:
    void             incomingConnection(qintptr cnxID);
    void             checkForDeadSenders(void);

private:
    bool             readNextTag(qint32 *tag);
    void             saveBackground(void);
    void             sendDesktopToClientsLk(void);

    void             applyPenDrawActions(QImage *img, QImage *highlightImg, pens_state_t *pens, bool penPresent[], int xLoc[], int yLoc[]);
    int              applyActionToImage(QImage *img, QImage *highLightImg, pens_state_t *pens, QColor &backColour, QVector<quint32> &actions, int startIndex);   // Return true if need another word to carry out action
    void             reRenderTextLayerImage(void);
    int              findLastUndoableActionForPen(int penNum);
    int              findLastDrawActionForPen(int penNum, int stopLoc); // Not do-agains
    void             deleteActionsForPen(int penNum, int readPt);
    void             undeleteActionsForPen(int penNum, int readPt);
    bool             doBrushFromLast(int penNum);
    bool             doBrushFromLast(int penNum, int endIndex);
    void             doCurrentBrushAction(int penNum, int x, int y);
    void             storeAndSendPreviousActionInt(quint32 data);
    void             storeAllActionIntLk(quint32 sendData);
    void             sendCurrentPageState(ViewServer *sender, int pageNum);
    int              w;                 // Allows us to reset size after playback
    int              h;
    QImage          *annotations;        // Image of all the annotations on the screen
    QImage          *highlights;
    QImage          *textLayerImage;
//    pens_state_t     currentPens[MAX_SCREEN_PENS];

    // Undo/Redo
    QList<QPoint>    penBrush[MAX_SCREEN_PENS];
    QPoint           penBrushCenter[MAX_SCREEN_PENS];

    // For text layer
    QVector<QString>           textPane;
    QVector< QVector<QColor> > textColour;
    QFont            *defaultFont;
    int               currentFontHeight;
    int               currentFontWidth;
    double            fontHeightScaling;

//    pens_state_t          replayBasePens[MAX_SCREEN_PENS];
    pens_state_t            replayCurrentPens[MAX_SCREEN_PENS];
    QColor                  defaultColour[MAX_SCREEN_PENS];
    QColor                  defaultBackgroundColour;
    QColor                  replayCurrentBackgroundColour;

    int                     penPosPrevX[MAX_SCREEN_PENS];
    int                     penPosPrevY[MAX_SCREEN_PENS];

    int                     currentpageNum;       // Start at 0
    QVector<pageData_t>     page;
    QVector<quint32>        rxActions;

    bool                    canBeServer;
#ifndef TABLET_MODE
    viewServerListener     *listener;
    QList<ViewServer *>     server;

    screenGrabber          *screenGrabTask;
#endif
    QWidget                *parentWindow;
};

#endif // OVERLAY_H
