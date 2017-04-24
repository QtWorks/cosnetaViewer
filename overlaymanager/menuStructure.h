#ifndef MENUSTRUCTURE_H
#define MENUSTRUCTURE_H

#include <QWidget>
#include <QColor>
#include <QImage>

#include "replayManager.h"
#include "overlaywindow.h"
#include "overlay.h"
#include "penStates.h"

typedef enum
{
    MINIMAL_LIST,
    ROTATING_SELECTION

} menuDisplayStyle_t;



class menuStructure : public QWidget
{
    Q_OBJECT

public:
    menuStructure(QWidget *parent, int pen, penCursor *ourCursor,
                  overlay *olay, replayManager *replayController);

    void hostOptions(hostOptions_t opt);
    void makePeer(void);
    void makeFollower(void);
    void makeSessionLeader(void);

    void drawMinimalList(void);
    void drawRotatingSelection(void);

    void resetMenu(void);
    bool updateWithInput(int x, int y, buttonsActions_t *buttons);
    void moveCentre(int x, int y);

    // Force values to allow an initial state to be created
    void setColour(QColor newColour);
    void setThickness(int newThickness);
    void setDrawStyle(penAction_t newDrawStyle);

protected:
    void paintEvent(QPaintEvent *pev);

private:
    void doStorageAction(storageAction_t action);
    void renderIconForCircleSize(menuEntry_t *menu, int size);
    void applyEditAction(pen_edit_t action);
    void pageAction(pageAction_t action);
    void stickyAction(stickyAction_t action);
    void resizeAllIconSets(void);
    void advanceToValidEntry(int &entryNum);

    // References to allow us to kick events off
    overlay             *overlayManager;
    replayManager       *replay;
    class overlayWindow *mainWindow;
    penCursor           *cursor;

    int                  penNum;    // Required for our access to overlay & replay

    // Data specific to this pen alone
    QColor              colour;       // Custom or preset
    int                 thickness;
    penAction_t         drawStyle;    // Normal/dashed/dotted/dot-dashed/HighLight/Erase
    pen_edit_t          editAction;   // Undo/Redo/Do-again

    penRole_t           role;         // Leader/Follower/peer

    pen_settings_t      penSettings;

    // Internal menu position data
    menuSet_t           *currentMenu;
    int                  currentEntry;
    QList<menuEntry_t *> currentParentEntry;
    int                  circleSz;

    // Display options
    menuDisplayStyle_t  style;
    bool                redrawRequired;

    QImage              menuImage;
};

#endif // MENUSTRUCTURE_H
