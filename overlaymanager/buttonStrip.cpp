#include <QVBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QVariant>
#include <QPainter>
#include <QSvgRenderer>
#include <QIcon>
#include <QImage>
#include <QDebug>

#include "buttonStrip.h"

#include "overlaywindow.h"

// Normal menus (used in

// View mode - no menus in pinch-zoom/pan :)
// Mouse mode - buttons mapping to mouse buttons for touch screens & inputs with <3 mouse buttons
menuSet_t mouseControls;
// Presentation controller - buttons describing the actions
menuSet_t presentationControls;

// Show/Hide
menuEntry_t showMenuEntry   = {":/menus/images/top_level_show_menu_vector.svg", NULL, "Show Menu",   SESSION_ALL|PAGE_TYPE_ALL, TOGGLE_MENU, NULL, NULL, NULL };
menuEntry_t hideMenuEntry   = {":/menus/images/top_level_hide_menu_vector.svg", NULL, "Hide Menu",   SESSION_ALL|PAGE_TYPE_ALL, TOGGLE_MENU, NULL, NULL, NULL };
menuEntry_t menuParentEntry = {":/images/menu/go_to_top_level_vector.svg",     NULL, "Parent Menu", SESSION_ALL|PAGE_TYPE_ALL, MENU_PARENT, NULL, NULL, NULL };

// Interface type
menuEntry_t viewControllerEntry  = {":/images/tablet/pan_zoom.svg",     NULL, "Zoom/Pan View",  SESSION_ALL|PAGE_TYPE_ALL, INPUT_IS_VIEW_CONTROL,      NULL, NULL, NULL };
menuEntry_t overlayPenEntry      = {":/images/tablet/overlay.svg",      NULL, "Annotate",       SESSION_ALL|PAGE_TYPE_ALL, INPUT_IS_OVERLAY_DRAW,      NULL, NULL, NULL };
menuEntry_t mouseKeyboardEntry   = {":/images/tablet/mouse_mode.svg",   NULL, "Mouse/Keyboard", SESSION_ALL|PAGE_TYPE_ALL, INPUT_IS_PRESENTATION_CTRL, NULL, NULL, NULL };
menuEntry_t presentationEntry    = {":/images/tablet/presentation.svg", NULL, "Presentation",   SESSION_ALL|PAGE_TYPE_ALL, INPUT_IS_MOUSE,             NULL, NULL, NULL };

// Mouse mode
//menuEntry_t leftClickEntry       = {":/images/top/undo.svg",     NULL, "Left Mouse",       SESSION_ALL|PAGE_TYPE_ALL, EDIT_ACTION, (void *)EDIT_UNDO, NULL, NULL };
menuEntry_t middleClickEntry     = {":/images/tablet/middle_mouse.svg",    NULL, "Middle Mouse",     SESSION_ALL|PAGE_TYPE_ALL, MIDDLE_MOUSE_PRESS,     NULL, NULL, NULL };
menuEntry_t rightClickEntry      = {":/images/tablet/right_mouse.svg",     NULL, "Right Mouse",      SESSION_ALL|PAGE_TYPE_ALL, RIGHT_MOUSE_PRESS,      NULL, NULL, NULL };
menuEntry_t scrollUpClickEntry   = {":/images/top/undo.svg",               NULL, "ScrollUp Mouse",   SESSION_ALL|PAGE_TYPE_ALL, MOUSE_SCROLLUP_PRESS,   NULL, NULL, NULL };
menuEntry_t scrollDownClickEntry = {":/images/top/undo.svg",               NULL, "ScrollDown Mouse", SESSION_ALL|PAGE_TYPE_ALL, MOUSE_SCROLLDOWN_PRESS, NULL, NULL, NULL };
menuEntry_t sendTextEntry        = {":/images/tablet/keyboard.svg",        NULL, "Keyboard Input",   SESSION_ALL|PAGE_TYPE_ALL, TOGGLE_KEYBOARD_INPUT,  NULL, NULL, NULL };

menuEntry_t buttonDisconnectEntry = {":/settings/images/settings/quit.svg", NULL, "Disconnect",
                                     SESSION_ALL|PAGE_TYPE_ALL, DISCONNECT_FROM_HOST, NULL,            NULL, NULL };

// Presentation controller mode
menuEntry_t nextSlideEntry           = {":/images/presentation/next_slide.svg",                NULL, "Next Slide",           SESSION_ALL|PAGE_TYPE_ALL, PRESENTATION_CONTROL, (void *)NEXT_SLIDE,     NULL, NULL };
menuEntry_t lastSlideEntry           = {":/images/presentation/previous_slide.svg",            NULL, "Last Slide",           SESSION_ALL|PAGE_TYPE_ALL, PRESENTATION_CONTROL, (void *)PREV_SLIDE,     NULL, NULL };
menuEntry_t highlightToggleEntry     = {":/images/presentation/toggle_laser.svg",              NULL, "Toggle highlight",     SESSION_ALL|PAGE_TYPE_ALL, PRESENTATION_CONTROL, (void *)SHOW_HIGHLIGHT, NULL, NULL };
menuEntry_t startPresentationEntry   = {":/images/presentation/start_presentation.svg",        NULL, "Start Presentation",   SESSION_ALL|PAGE_TYPE_ALL, PRESENTATION_CONTROL, (void *)START_SHOW,     NULL, NULL };
menuEntry_t stopPresentationEntry    = {":/images/presentation/stop_presentation.svg",         NULL, "Stop Presentation",    SESSION_ALL|PAGE_TYPE_ALL, PRESENTATION_CONTROL, (void *)STOP_SHOW,      NULL, NULL };
menuEntry_t defaultPresentationEntry = {":/images/presentation/open_default_presentation.svg", NULL, "Default Presentation", SESSION_ALL|PAGE_TYPE_ALL, PRESENTATION_CONTROL, (void *)START_DEFAULT,  NULL, NULL };


// Overlay mode menu options
extern menuSet_t topLevel;



#ifdef SCROLLAREA_BUTTONS

// Scrollarea implementation using buttons

buttonStrip::buttonStrip(int pseudoPenNum, QWidget *parent) : QWidget(parent)
{
    parentWidget  = parent;
    penNum        = pseudoPenNum;

    setMinimumSize(parent->width(),parent->height());
//    setAttribute( Qt::WA_TranslucentBackground, true);
    setStyleSheet("background-color: transparent;");

    scroll = new QScrollArea(this);
    scroll->setStyleSheet("background-color: transparent;");
    scroll->setMinimumSize(parent->width(),parent->height());
    scroll->setAlignment(Qt::AlignVCenter|Qt::AlignRight);
    scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scroll->setWidgetResizable(false);

    buttonGrid = new QGridLayout(scroll);
    mainLayout = new QVBoxLayout;
    mainLayout->addWidget(scroll);
    setLayout(mainLayout);

    // Set up menus
    createExtendedMenuTreeOfButtons();

    // And the current menu state
    setInitialButtonMenuState();
    setMenuMode( viewMode );

    connect(&buttonGroup,SIGNAL(buttonClicked(int)), this,SLOT(buttonGroupClicked(int)) );

    // Start hidden
    hide();
}

void buttonStrip::createExtendedMenuTreeOfButtons(void)
{
    // Add the top level as a vertical set of buttons
    showMenuButton = buttonFromMenuEntry(&showMenuEntry,ICON_SIZE/3);
    hideMenuButton = buttonFromMenuEntry(&hideMenuEntry,ICON_SIZE/3);

    overlayMenuButton       = buttonFromMenuEntry(&overlayPenEntry,ICON_SIZE);
    mouseKeyboardMenuButton = buttonFromMenuEntry(&mouseKeyboardEntry,ICON_SIZE);
    presentationMenuButton  = buttonFromMenuEntry(&presentationEntry,ICON_SIZE);
    disconnectMenuButton    = buttonFromMenuEntry(&buttonDisconnectEntry,ICON_SIZE);

    buttonGrid->addWidget(showMenuButton,0,0);

    // Presentation and mouse controls

    // Create entries for all the nested menus (annotate mode)
}

void buttonStrip::setInitialButtonMenuState(void)
{
    // Set variables and visibility of the menus
    buttonStripIsVisible = false;

    // Hide most stuff

    // Explicitly hide the top level button items
    overlayMenuButton->hide();
    mouseKeyboardMenuButton->hide();
    presentationMenuButton->hide();
    disconnectMenuButton->hide();

    showMenuButton->show();
    hideMenuButton->hide();
}


void buttonStrip::appendWidgetsForMenu(menuSet_t *start)
{
    // Walk the tree
    for(int i=0; i<start->entry.size(); i++)
    {
//        qDebug() << "appendWidget" << i << QString(start->entry[i].altText);

        // Record parent in the menu entry too
        start->entry[i].parentMenuset = start;

        // Add a button for the menu (note this is a collapse point)
        buttonFromMenuEntry( &(start->entry[i]),ICON_SIZE );

        // Recursive call to add the children immediatly following the menu button...
        if( start->entry[i].type == CHILD_MENU )
        {
            // And add the children
            if( start->entry[i].data != NULL )
            {
                appendWidgetsForMenu(((menuSet_t *)(start->entry[i].data)) );
            }
        }
    }
}

QToolButton *buttonStrip::buttonFromMenuEntry(menuEntry_t *entry, int iconSize)
{
    QPixmap iconPixmap;

    if( entry->icon_resource.length()>1 )
    {
        QImage  iconImage(iconSize,iconSize, QImage::Format_ARGB32_Premultiplied);
        iconImage.fill(Qt::transparent);

        QSvgRenderer re;
        QPainter iconPaint(&iconImage);
        iconPaint.setRenderHint(QPainter::Antialiasing);

        re.load(entry->icon_resource);
        re.render(&iconPaint);

        iconPaint.end();

        iconPixmap = QPixmap::fromImage(iconImage);
    }
    else if( entry->type == PEN_COLOUR_PRESET || entry->type == BACKGROUND_COLOUR_PRESET )
    {
        QImage  iconImage(iconSize,iconSize, QImage::Format_ARGB32_Premultiplied);
        iconImage.fill(Qt::transparent);

        QPainter iconPaint(&iconImage);

        iconPaint.setBrush( *((QColor *)(entry->data)) );
        iconPaint.setPen( QPen(Qt::darkGray,3) );
        iconPaint.drawEllipse(0,0, iconSize,iconSize );

        iconPixmap = QPixmap::fromImage(iconImage);
    }

    QIcon icon;
    icon.addPixmap(iconPixmap);

    QToolButton *button = new QToolButton;
    if( entry->icon_resource.length()>1 )
    {
        button->setIcon(icon);
    }
    QFont font = button->font();
    font.setPixelSize(8);
    button->setFont(font);
    button->setText(entry->altText);
    button->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    button->setIconSize(QSize(iconSize,iconSize));

    if(entry->type == CHILD_MENU )
    {
        button->setArrowType(Qt::LeftArrow);
    }
    else
    {
        button->setArrowType(Qt::NoArrow);
    }

    int newPoint = buttonTable.length();
    buttonGroup.addButton(button,newPoint);

    buttonTable.append( entry );
    buttons.append(button);

    return button;
}


void buttonStrip::screenResized(int newWidth, int newHeight)
{
    qDebug() << "set screen size to:" << newWidth << newHeight;

    setMinimumSize(newWidth, newHeight);
    scroll->setMinimumSize(newWidth, newHeight);
    mainLayout->setGeometry(QRect(0,0,newWidth, newHeight));

//    resize(QSize(ICON_SIZE,parentWidget->height()));
//    move(parentWidget->width()-width(),0);
}

void buttonStrip::updateVisibleButtonsFromRoleAndSession(void)
{
    qDebug() << "TODO: updateVisibleButtonsFromRoleAndSession";
}

void buttonStrip::buttonGroupClicked(int buttonIdx)
{
    menuEntry_t *menuEntry = buttonTable[buttonIdx];

    qDebug() << "*** CALLED buttonGroupClicked("<< buttonIdx << ") type" << menuEntry->type << "***";

    switch( menuEntry->type )
    {
    case TOGGLE_MENU:
        toggleButtonStripVisibility();
        return;

    case CHILD_MENU:
        return;

    case INPUT_IS_VIEW_CONTROL:
        return;

    case INPUT_IS_OVERLAY_DRAW:
        return;

    case INPUT_IS_PRESENTATION_CTRL:
        return;

    case INPUT_IS_MOUSE:
        return;
    }

    buttonAction(menuEntry);
}

void buttonStrip::toggleButtonStripVisibility(void)
{
    qDebug() << "*** CALLED toggleButtonStripVisibility() ***";

    if( buttonStripIsVisible )
    {
        buttonStripIsVisible = false;

        // Top level for now (will surrond with a test)
        hideMenuButton->hide();
        buttonGrid->removeWidget(hideMenuButton);
        buttonGrid->addWidget(showMenuButton,0,0);
        showMenuButton->show();
    }
    else
    {
        buttonStripIsVisible = true;

        // Top level for now (will surrond with a test)
        showMenuButton->hide();
        buttonGrid->removeWidget(showMenuButton);
        buttonGrid->addWidget(hideMenuButton,0,0);
        hideMenuButton->show();
    }

    update();
}


void buttonStrip::setMenuMode(current_menu_t newMode)
{
    currentMode = newMode;

    qDebug() << "TODO: change mode view";
}

#else

// QWidgetList implementation
buttonStrip::buttonStrip(int pseudoPenNum, QWidget *parent) : QWidget(parent)
{
    parentWidget  = parent;
    penNum        = pseudoPenNum;

    lastOpenPoint = NULL;

    resize(QSize(ICON_SIZE+5,parentWidget->height()-20));
    move(parentWidget->width()-width(),20);

    widgetList = new QListWidget(this);
    widgetList->resize(QSize(ICON_SIZE,parentWidget->height()-20));
    widgetList->setViewMode(QListView::IconMode);  // Alternative is ListMode, which is small icon next to text
    widgetList->setUniformItemSizes(true);
    widgetList->setSpacing(0);
    widgetList->setMovement(QListView::Static);
    widgetList->setFrameShape(QFrame::StyledPanel);
    widgetList->setFrameShadow(QFrame::Sunken);
    widgetList->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    widgetList->setMinimumHeight(parentWidget->height());
    widgetList->setStyleSheet("background-color: transparent;");

    QLayout *listLayout = widgetList->layout();
    if( listLayout == NULL )
    {
        QVBoxLayout *vbl = new QVBoxLayout;
        vbl->setAlignment(Qt::AlignCenter);
        widgetList->setLayout(vbl);
    }
    else listLayout->setAlignment(Qt::AlignCenter);

    // Create local menu structure
    createExtendedMenuTreeOfButtons();

    // Add the local button menus buttons
    currentMode = viewMode;
#ifdef Q_OS_ANDROID
    addWidgetListItemAsButton(&viewControllerEntry);
    addWidgetListItemAsButton(&buttonDisconnectEntry);
    currentMode = overlayMode;
    addWidgetListItemAsButton(&overlayPenEntry);
    appendWidgetsForMenu(&topLevel);
    currentMode = presenterMode;
    addWidgetListItemAsButton(&presentationEntry);
    appendWidgetsForMenu(&presentationControls);
    currentMode = mouseMode;
    addWidgetListItemAsButton(&mouseKeyboardEntry);
    appendWidgetsForMenu(&mouseControls);
#else
    appendWidgetsForMenu(&topLevel);
#endif

    currentMode       = NODE_NOT_SET;
    atTopLevel        = true;
    keyboardAvailable = false;
    role              = SESSION_PEER;
    sessionType       = PAGE_TYPE_OVERLAY;

    // Start in pan/zoom mode
#ifdef Q_OS_ANDROID
    setMenuMode(viewMode);
#else
    setMenuMode(overlayMode);
#endif
    // Connect the button click event
    connect(widgetList, SIGNAL(itemClicked(QListWidgetItem *)),
            this,       SLOT(optionClicked(QListWidgetItem *)) );

    // Start hidden
    hide();
}


void buttonStrip::screenResized(int newWidth, int newHeight)
{
    qDebug() << "set screen size to:" << newWidth << newHeight;

    resize(QSize(ICON_SIZE,parentWidget->height()-20));
    move(parentWidget->width()-width(),20);
    widgetList->resize(QSize(ICON_SIZE,height()-20));
}


void buttonStrip::setMenuMode(current_menu_t newMode)
{
    overlayWindow *mw = (overlayWindow *)parentWidget;

    switch( newMode )
    {
    case viewMode:
    case presenterMode:
#ifdef Q_OS_ANDROID
        if( keyboardAvailable )
        {
            keyboardAvailable = false;
            mw->enableTextInputWindow(keyboardAvailable);
        }
#endif
    case overlayMode:
    case mouseMode:
        break;          // Allowable new modes

    default:
        qDebug() << "buttonStrip::setMenuMode(): Bad newMode. Ignoring:" << newMode;
        return;
    }

    qDebug() << "Overlay window instance =" << mw;
    mw->setZoomPanModeOn( newMode == viewMode );

    // Hide the current menus
    QVector<QListWidgetItem *> list;

    qDebug() << "Hide all of old mode";

    switch(currentMode)
    {
    case viewMode:      list = viewModeWidgetItems;      break;
    case overlayMode:   list = overlayModeWidgetItems;   break;
    case presenterMode: list = presenterModeWidgetItems; break;
    case mouseMode:     list = mouseModeWidgetItems;     break;
    case NODE_NOT_SET:  break;  // This is the first pass.
    default:            return;
    }

    qDebug() << "Hide old mode. List length =" << list.size();
    for(int it=0; it<list.size(); it++)
    {
        list[it]->setHidden(true);
    }

    // Set the new mode
    currentMode = newMode;

    qDebug() << "Show new mode";

    // show the top level of the new menus
    switch(currentMode)
    {
    case viewMode:      list = viewModeWidgetItems;      break;
    case overlayMode:   list = overlayModeWidgetItems;   break;
    case presenterMode: list = presenterModeWidgetItems; break;
    case mouseMode:     list = mouseModeWidgetItems;     break;
    default:            return;
    }

    // And tell the host of our new mode
    switch(currentMode)
    {
    case viewMode:      // Set overlay mode so remote end's mouse is released.
    case overlayMode:   mw->setPenMode(penNum,APP_CTRL_PEN_MODE_OVERLAY);     break;
    case presenterMode: mw->setPenMode(penNum,APP_CTRL_PEN_MODE_PRESNTATION); break;
    case mouseMode:     mw->setPenMode(penNum,APP_CTRL_PEN_MODE_MOUSE);       break;
    }

    // Show the top level item
    qDebug() << "Show new mode. List length =" << list.size();
    if( list.size() < 1 )
    {
        return;
    }
    list[0]->setHidden(false);

    // Show the top level (don't show anything for view/pan)
    if( currentMode != viewMode )  showChildItemsAtSameDepth(list[0]);
    else                           ((QListWidgetItem *)buttonDisconnectEntry.buttonWidget)->setHidden(false);

    atTopLevel = true;
}

current_menu_t buttonStrip::getMenuMode(void)
{
    return currentMode;
}


void buttonStrip::createExtendedMenuTreeOfButtons(void)
{
    // Mouse mode
//    mouseControls.entry.append(leftClickEntry);
    mouseControls.entry.append(middleClickEntry);
    mouseControls.entry.append(rightClickEntry);
//    mouseControls.entry.append(scrollUpClickEntry);
//    mouseControls.entry.append(scrollDownClickEntry);
    mouseControls.entry.append(sendTextEntry);
    mouseControls.entry.append(buttonDisconnectEntry);
    mouseControls.circleSizeForIconRender = 0;
    mouseControls.parent                  = NULL;

    // Presentation controller mode
    presentationControls.entry.append(nextSlideEntry);
    presentationControls.entry.append(lastSlideEntry);
    presentationControls.entry.append(highlightToggleEntry);
    presentationControls.entry.append(startPresentationEntry);
    presentationControls.entry.append(stopPresentationEntry);
    presentationControls.entry.append(defaultPresentationEntry);
    presentationControls.entry.append(buttonDisconnectEntry);
    presentationControls.circleSizeForIconRender = 0;
    presentationControls.parent                  = NULL;
}


void buttonStrip::appendWidgetsForMenu(menuSet_t *start)
{
    // Walk the tree
    for(int i=0; i<start->entry.size(); i++)
    {
//        qDebug() << "appendWidget" << i << QString(start->entry[i].altText);

        // Record parent in the menu entry too
        start->entry[i].parentMenuset = start;

        // Add a button for the menu (note this is a collapse point)
        addWidgetListItemAsButton( &(start->entry[i]) );

        // Recursive call to add the children immediatly following the menu button...
        if( start->entry[i].type == CHILD_MENU )
        {
            // And add the children
            if( start->entry[i].data != NULL )
            {
                appendWidgetsForMenu(((menuSet_t *)(start->entry[i].data)) );
            }
        }
    }
}


void buttonStrip::addWidgetListItemAsButton(menuEntry_t *entry)
{
    QListWidgetItem *newItem = new QListWidgetItem(widgetList, QListWidgetItem::UserType);

    // Create a button image
    QPixmap iconPixmap;

    if( entry->icon_resource.length()>1 )
    {
        QImage  iconImage(ICON_SIZE,ICON_SIZE, QImage::Format_ARGB32_Premultiplied);
        iconImage.fill(Qt::transparent);

        QSvgRenderer re;
        QPainter iconPaint(&iconImage);
        iconPaint.setRenderHint(QPainter::Antialiasing);

        iconPaint.setBrush(Qt::gray);
        iconPaint.drawEllipse(0,0,ICON_SIZE,ICON_SIZE);

        re.load(entry->icon_resource);
        re.render(&iconPaint);

        iconPaint.end();

        iconPixmap = QPixmap::fromImage(iconImage);
    }
    else if( entry->type == PEN_COLOUR_PRESET || entry->type == BACKGROUND_COLOUR_PRESET )
    {
        QImage  iconImage(ICON_SIZE,ICON_SIZE, QImage::Format_ARGB32_Premultiplied);
        iconImage.fill(Qt::transparent);

        QPainter iconPaint(&iconImage);

        iconPaint.setBrush( *((QColor *)(entry->data)) );
        iconPaint.setPen( QPen(Qt::darkGray,3) );
        iconPaint.drawEllipse(0,0, ICON_SIZE,ICON_SIZE );

        iconPixmap = QPixmap::fromImage(iconImage);
    }

    // Add the icon
    newItem->setIcon(QIcon(iconPixmap));

    if( entry->type == CHILD_MENU )
    {
        // Add a visual indication common to all openable menus
        newItem->setFlags(Qt::ItemIsUserCheckable|Qt::ItemIsEnabled);
    }
    else
    {
        // And what does a plain button look like - no bling for now
        newItem->setFlags(Qt::ItemIsEnabled);
    }

    // And the font
    QFont  itemFont = newItem->font();
    itemFont.setPixelSize(8);
    newItem->setFont(itemFont);

    // Add the non-icon data to the new ListItem
    newItem->setTextAlignment(Qt::AlignCenter|Qt::AlignJustify);
    newItem->setData(Qt::UserRole, QVariant(QMetaType::VoidStar,((void *)&entry)));
    newItem->setText(entry->altText);
    newItem->setSizeHint(QSize(ICON_SIZE,ICON_SIZE));
    newItem->setHidden(true);

    // Record this widget in the menu data structure
    entry->buttonWidget = newItem;

    // Add this item to the appropriate list
    switch(currentMode)
    {
    case viewMode:      viewModeWidgetItems.append(newItem);      break;
    case overlayMode:   overlayModeWidgetItems.append(newItem);   break;
    case presenterMode: presenterModeWidgetItems.append(newItem); break;
    case mouseMode:     mouseModeWidgetItems.append(newItem);     break;
    default:                                                      break;
    }
}


// User interaction!
void buttonStrip::optionClicked(QListWidgetItem *item)
{
    // We're not using selections with this, so clear any
    widgetList->clearSelection(); // Doesn't seem to work though :(

    // Manual: top button - each press should just cycle through the modes
    // Note that if not at the top level, pressing the "cycle" icon that
    // is displaying the current mode will instead take you to the top level.
    if(item == viewModeWidgetItems[0])
    {
        overlayWindow *mw = (overlayWindow *)parentWidget;

        if( mw->connectToRemoteForEdit() ) setMenuMode(overlayMode); // No sub-menus, so no test.

        return;
    }
    else if( item == overlayModeWidgetItems[0] )
    {
        if( atTopLevel )  setMenuMode(presenterMode);
        else              setMenuMode(overlayMode); // Go to top level of this mode

        return;
    }
    else if( item == presenterModeWidgetItems[0] )
    {
        if( atTopLevel )  setMenuMode(mouseMode);
        else              setMenuMode(presenterMode); // Go to top level of this mode

        return;
    }
    else if( item == mouseModeWidgetItems[0] )
    {
        if( atTopLevel )  setMenuMode(viewMode);
        else              setMenuMode(mouseMode); // Go to top level of this mode

        overlayWindow *mw = (overlayWindow *)parentWidget;
        mw->disconnectFromRemoteEdit();

        return;
    }

    // Check for a menu open/close
    QVariant     menuEntryVal = item->data(Qt::UserRole);
    menuEntry_t *menuEntry    = (menuEntry_t *)( menuEntryVal.value<void *>() );
    if( menuEntry->type == CHILD_MENU )
    {
        showChildItemsAtSameDepth(item);
        atTopLevel = false;
        return;
    }

    buttonAction(menuEntry);
}


void buttonStrip::showChildItemsAtSameDepth(QListWidgetItem *openPoint)
{
    if( currentMode == NODE_NOT_SET ) return;

    lastOpenPoint = openPoint;

    updateVisibleButtonsFromRoleAndSession();
}

void buttonStrip::updateVisibleButtonsFromRoleAndSession(void)
{
    if( lastOpenPoint == NULL )
    {
        qDebug() << "lastOpenPoint = NULL!";
        return;
    }
    // Hide the current menus
    QVector<QListWidgetItem *> list;

    qDebug() << "Show list items starting at" << lastOpenPoint << "=" << lastOpenPoint->text();

    switch(currentMode)
    {
    case viewMode:      list = viewModeWidgetItems;      break;
    case overlayMode:   list = overlayModeWidgetItems;   break;
    case presenterMode: list = presenterModeWidgetItems; break;
    case mouseMode:     list = mouseModeWidgetItems;     break;
    case NODE_NOT_SET:
    default:            return;
    }

    int listEntry = 0;

    // Hide everything

    qDebug() << "Hide everything (except top button).";

    for( int idx=1; idx<list.size(); idx ++ )
    {
        list[idx]->setHidden(true);

        if( lastOpenPoint == list[idx] )
        {
            qDebug() << "Found listEntry << idx";

            listEntry = idx;
        }
    }

    // Start point
    if( (listEntry+1) >= list.size() )
    {
        listEntry = list.size() - 2;
    }

    QVariant     menuEntryVal = list[listEntry+1]->data(Qt::UserRole);
    menuEntry_t *current      = (menuEntry_t *)( menuEntryVal.value<void *>() );

    qDebug() << "current =" << current << list[listEntry+1]->text()
             << "listEntry" << listEntry;

    // Show peers
    menuSet_t *parentItem = (menuSet_t *)(current->parentMenuset);

    qDebug() << "parentItem =" << parentItem << "show"
             << parentItem->entry.size()     << "entries.";

    overlayWindow *mw = (overlayWindow *)parentWidget;
    for(int idx=0; idx<parentItem->entry.size(); idx ++)
    {
        QListWidgetItem *w = (QListWidgetItem *)parentItem->entry[idx].buttonWidget;

        if( 0 != (parentItem->entry[idx].validRoles & sessionType) &&
            0 != (parentItem->entry[idx].validRoles & role)                       )
        {
            qDebug() << "Show peer" << w << w->text();

            w->setHidden(false);
        }
        else
        {
            qDebug() << "Skip" << idx << w->text() << "due to role. overlay session ="
                     << QString("0x%1").arg(mw->currentSessionType(),0,16)
                     << "entry roles ="
                     << QString("0x%1").arg(parentItem->entry[idx].validRoles,0,16);
        }
    }

    // Show parent breadcrumbs

    qDebug() << "Show parents";

    current    = (menuEntry_t *)( menuEntryVal.value<void *>() );
    menuSet_t *currentParent = (menuSet_t *)(current->parentMenuset);
    parentItem = currentParent->parent;

    while( parentItem != NULL )
    {
        // Search through parent menu for entry pointing to current menuSet
        for(int idx=0; idx<parentItem->entry.size(); idx ++)
        {
            // Only use mode, not current role as that's for the host to decide.
            // It will just reject a command of you are not allowed to do it. (TODO)
            if( parentItem->entry[idx].data == currentParent )
            {
                QListWidgetItem *w = (QListWidgetItem *)parentItem->entry[idx].buttonWidget;

                if( w == NULL ) { qDebug() << "NULL POINTER ALERT"; break; }

                w->setHidden(false);

                qDebug() << "Show parent" << w << w->text();

                currentParent = parentItem;

                break;
            }
        }

        // And up a level
        parentItem = parentItem->parent;
    }

    widgetList->setEnabled(true);

    update();
}

#endif

void buttonStrip::buttonAction(menuEntry_t *menuEntry)
{
    // Menu click to actually do stuff!!!

    overlayWindow *mw = (overlayWindow *)parentWidget;

    // Do stuff depending on the action type:

    QColor     colour;
    int        thickness;
    pen_edit_t editAction;

    switch( menuEntry->type )
    {
    case PEN_COLOUR_PRESET:
        colour = *((QColor *)(menuEntry->data));
        mw->penColourChanged(penNum,colour);
        break;

    case BACKGROUND_COLOUR_PRESET:
        colour = *((QColor *)(menuEntry->data));
        mw->paperColourChanged(penNum, *((QColor *)(menuEntry->data)) );
        break;


    case THICKNESS_PRESET:
        thickness = (long)(menuEntry->data);
        mw->penThicknessChanged(penNum,thickness);
        break;

    case PEN_TYPE:
        mw->setPenType(penNum, (penAction_t)((long)(menuEntry->data)) );
        if(keyboardAvailable == true) {
            keyboardAvailable = false;
            mw->enableTextInputWindow(keyboardAvailable);
        } else {
            mw->enableTextInputWindow(keyboardAvailable);
        }
        break;

    case PEN_SHAPE:
        mw->setPenShape(penNum, (shape_t)((long)(menuEntry->data)) );
        break;

    case EDIT_ACTION:
        editAction = ((pen_edit_t)((long)(menuEntry->data)));
        switch( editAction )
        {
        case EDIT_UNDO:     mw->undoPenAction(penNum);  break;
        case EDIT_REDO:     mw->redoPenAction(penNum);  break;
        case EDIT_DO_AGAIN: mw->brushFromLast(penNum);  break;
        default:                                        break;
        }

        break;

    case PAGE_ACTION:
        switch( ((pageAction_t)((long)(menuEntry->data))) )
        {
        case PAGE_NEW:      mw->newPage();          break;
        case PAGE_NEXT:     mw->nextPage();         break;
        case PAGE_PREV:     mw->prevPage();         break;
        case PAGE_SELECTOR: mw->showPageSelector(); break;
        case PAGE_CLEAR:    mw->clearOverlay();     break;
        }

        break;
#ifdef Q_OS_ANDROID
    case TOGGLE_KEYBOARD_INPUT:
        if(keyboardAvailable == false) {
            keyboardAvailable = true;
            mw->enableTextInputWindow(keyboardAvailable);
        } else {
            mw->enableTextInputWindow(keyboardAvailable);
        }
        mw->setPenType(penNum, DRAW_TEXT);
        break;
#endif
    case STICKY_ACTION:
        switch((stickyAction_t)((long)(menuEntry->data)))
        {
        case STICKY_NEW:     mw->addNewStickyStart(penNum);     break;
        case STICKY_DELETE:  mw->deleteStickyMode(penNum);      break;
        case STICKY_MOVE:    mw->startmoveofStickyNote(penNum); break;

        case STICKY_SNAP:
        case STICKY_ARRANGE:
            qDebug() << "Sticky actions: STICKY_SNAP, STICKY_ARRANGE not implemented";
            break;
        }

        break;

    case HOST_OPTIONS:
        switch(((hostOptions_t)(long)(menuEntry->data)))
        {
        case CONTROL_PEN_ACCESS:      break;
        case HOSTED_MODE_TOGGLE:      mw->hostedModeToggle(penNum);          break;
        case REMOTE_SESSION:          mw->remoteNetSelectAndConnect(penNum); break;
        case ALLOW_REMOTE:            mw->allowRemoteNetSession(penNum);     break;
        case SWITCH_SCREEN:           break;
        case MOUSE_AS_LOCAL_PEN:      mw->toggleMouseAsPen();                break;
        case SESSION_TYPE_OVERLAY:    mw->sessionTypeOverlay(penNum);        break;
        case SESSION_TYPE_WHITEBOARD: mw->sessionTypeWhiteboard(penNum);     break;
        case SESSION_TYPE_STICKIES:   mw->sessionTypeStickyNotes(penNum);    break;
        }

        break;

    case STORAGE_ACTION:
        switch(((storageAction_t)(long)(menuEntry->data)))
        {
        case RECORD_SESSION:            mw->recordDiscussion();                  break;
        case PLAYBACK_RECORDED_SESSION: mw->playbackDiscussion();                break;
        case STORAGE_SNAPSHOT:          mw->screenSnap();                        break;
        case SNAPSHOT_GALLERY:         qDebug() << "Snapshot gallery: TODO";     break;
        case STORAGE_SAVE:             qDebug() << "Export to PDF here. Later."; break;
        }

        break;

    case LEFT_MOUSE_TOGGLE:             mw->setMouseButtonPulse(penNum, MOUSE_LEFT);       break;
    case MIDDLE_MOUSE_PRESS:            mw->setMouseButtonPulse(penNum, MOUSE_MIDDLE);     break;
    case RIGHT_MOUSE_PRESS:             mw->setMouseButtonPulse(penNum, MOUSE_RIGHT);      break;
    case MOUSE_SCROLLDOWN_PRESS:        mw->setMouseButtonPulse(penNum, MOUSE_SCROLLUP);   break;
    case MOUSE_SCROLLUP_PRESS:          mw->setMouseButtonPulse(penNum, MOUSE_SCROLLDOWN); break;

    case PRESENTATION_CONTROL:          mw->sendPresntationControl(penNum, (presentation_controls_t)(long)(menuEntry->data));
                                        break;

    case APPLICATION_HELP:              break;

    case DISCONNECT_FROM_HOST:          mw->mainMenuDisconnect();  break;
    case QUIT_APPLICATION:              mw->mainMenuQuit();        break;

    default:
        qDebug() << "Bad entryType: " << menuEntry->type;
    }

    update(); // For clear selection, not that it works.
}

// //////////////////////////////////////////////////
// Session type update to show/hide available options
void buttonStrip::makePeer(void)
{
    role = SESSION_PEER;

    updateVisibleButtonsFromRoleAndSession();
}

void buttonStrip::makeSessionLeader(void)
{
    role = SESSION_LEADER;

    updateVisibleButtonsFromRoleAndSession();
}

void buttonStrip::makeFollower(void)
{
    role = SESSION_FOLLOWER;

    updateVisibleButtonsFromRoleAndSession();
}

void buttonStrip::setSessionType(int newSessionType)
{
    sessionType = newSessionType;

    updateVisibleButtonsFromRoleAndSession();
}


