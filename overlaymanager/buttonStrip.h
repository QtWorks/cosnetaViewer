#ifndef BUTTONSTRIP_H
#define BUTTONSTRIP_H

#include <QWidget>
#include <QGroupBox>
#include <QToolButton>
#include <QButtonGroup>
#include <QSignalMapper>
#include <QStackedLayout>
#include <QScrollArea>

#include <QListWidget>
#include <QVector>

#include "penStates.h"

// Remove to use the QWidgetList implementation
// #define SCROLLAREA_BUTTONS

#define ICON_SIZE 80


typedef enum
{
    viewMode, privateMode, overlayMode, presenterMode, mouseMode, NODE_NOT_SET

} current_menu_t;

typedef struct
{
    QVector<QToolButton *> buttonWidget;
    int                    x, y;
    int                    selectedSubmenu;

} menuButtonsColumn_t;


class buttonStrip : public QWidget
{
    Q_OBJECT
public:
    explicit buttonStrip(int pseudoPenNum, QWidget *parent = 0);

    void            updateVisibleButtonsFromRoleAndSession(void);
    void            makePeer(void);
    void            makeFollower(void);
    void            makeSessionLeader(void);
    void            setSessionType(int newSessionType);
    void            createExtendedMenuTreeOfButtons(void);
    void            screenResized(int newWidth, int newHeight);
    void            setMenuMode(current_menu_t newMode);
    current_menu_t  getMenuMode(void);
    void            buttonAction(menuEntry_t *menuEntry);
    void            appendWidgetsForMenu(menuSet_t *start);

#ifdef SCROLLAREA_BUTTONS
    void            setInitialButtonMenuState(void);
    QToolButton    *buttonFromMenuEntry(menuEntry_t *entry, int iconSize);
#else
    void            addWidgetListItemAsButton(menuEntry_t *entry);
    void            showChildItemsAtSameDepth(QListWidgetItem *openPoint);
#endif

private slots:
#ifdef SCROLLAREA_BUTTONS
    void            buttonGroupClicked(int buttonIdx);
#else
    void            optionClicked(QListWidgetItem *item);
#endif

private:
    QWidget                   *parentWidget;

    int             penNum;
    int             sessionType;  // Overlay/Whiteboard/Stickies
    penRole_t       role;         // Leader/Follower/peer

#ifdef SCROLLAREA_BUTTONS
    QVector<menuEntry_t *> buttonTable;
    QVector<QToolButton *> buttons;
    QScrollArea           *scroll;
    QGridLayout           *buttonGrid;
    QVBoxLayout           *mainLayout;
    QButtonGroup           buttonGroup;

    QToolButton           *showMenuButton;  // Hard wired stuff
    QToolButton           *hideMenuButton;
    QToolButton           *overlayMenuButton;
    QToolButton           *mouseKeyboardMenuButton;
    QToolButton           *presentationMenuButton;
    QToolButton           *disconnectMenuButton;

    QVector<menuButtonsColumn_t> buttonColumn;

    bool                   buttonStripIsVisible;
    void                   toggleButtonStripVisibility(void);
#else
    QListWidget               *widgetList;
    QVector<QListWidgetItem *> viewModeWidgetItems;
    QVector<QListWidgetItem *> privateModeWidgetItems;                  //pn
    QVector<QListWidgetItem *> overlayModeWidgetItems;
    QVector<QListWidgetItem *> presenterModeWidgetItems;
    QVector<QListWidgetItem *> mouseModeWidgetItems;
    QListWidgetItem           *lastOpenPoint;
#endif
    current_menu_t  currentMode;
    bool            atTopLevel;
    bool            keyboardAvailable;
};

#endif // BUTTONSTRIP_H
