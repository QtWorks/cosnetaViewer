#include <QColor>
#include <QDebug>
#include <QImage>
#include <QSvgRenderer>

#include <math.h>

#include "menuStructure.h"
#include "replayManager.h"
#include "overlaywindow.h"

menuStructure::menuStructure(QWidget *parent, int pen, penCursor *ourCursor,
                             overlay *olay,   replayManager *replayController) :
                             QWidget(parent,
                                     Qt::FramelessWindowHint|Qt::WindowStaysOnTopHint)
{
    // Menu location
    currentMenu = NULL;
    currentParentEntry.clear();

    // References to other objects
    cursor           = ourCursor;
    overlayManager   = olay;
    replay           = replayController;
    mainWindow       = (overlayWindow *)parent;
    penNum           = pen;

    // Data
    role        = SESSION_PEER;
    colour      = Qt::magenta;
    thickness   = 8;
    drawStyle   = DRAW_SOLID;
    editAction  = EDIT_UNDO;

    cos_retreive_pen_settings(pen, &penSettings);
    style          = ROTATING_SELECTION; // could be: MINIMAL_LIST;
    redrawRequired = true;
    circleSz       = 96;

    hide();
}

// //////////////////////////////////////////////////
// Session type update to show/hide available options
void menuStructure::makePeer(void)
{
    role = SESSION_PEER;
}

void menuStructure::makeSessionLeader(void)
{
    role = SESSION_LEADER;
}

void menuStructure::makeFollower(void)
{
    role = SESSION_FOLLOWER;
}


void menuStructure::hostOptions(hostOptions_t opt)
{
    switch(opt)
    {
    case CONTROL_PEN_ACCESS:
        break;

    case HOSTED_MODE_TOGGLE:
        mainWindow->hostedModeToggle(penNum);
        currentMenu = NULL;
        break;

    case REMOTE_SESSION:
        mainWindow->remoteNetSelectAndConnect(penNum);
        currentMenu = NULL;
        break;

    case ALLOW_REMOTE:
        mainWindow->allowRemoteNetSession(penNum);
        currentMenu = NULL;
        break;

    case SWITCH_SCREEN:
        break;

    case MOUSE_AS_LOCAL_PEN:
        mainWindow->toggleMouseAsPen();
        currentMenu = NULL;
        break;

    case SESSION_TYPE_OVERLAY:
        mainWindow->sessionTypeOverlay(penNum);
        redrawRequired = true;
        currentMenu    = NULL;
        break;

    case SESSION_TYPE_WHITEBOARD:
        mainWindow->sessionTypeWhiteboard(penNum);
        redrawRequired = true;
        currentMenu    = NULL;
        break;

    case SESSION_TYPE_STICKIES:
        mainWindow->sessionTypeStickyNotes(penNum);
        redrawRequired = true;
        currentMenu    = NULL;
        break;
    }
}


// /////////////////////////////////
// Force values (for initialisation)
void menuStructure::setColour(QColor newColour)
{
    colour = newColour;
}

void menuStructure::setThickness(int newThickness)
{
    if( thickness>0 && thickness<255)
    {
        thickness = newThickness;
    }
}

void  menuStructure::setDrawStyle(penAction_t newDrawStyle)
{
    // Only update if it's valid.

    switch( newDrawStyle )
    {
    case DRAW_TEXT:
        qDebug() << "menuStructure::setDrawStyle() DRAW_TEXT - TODO";
        break;

    case DRAW_SOLID:
    case DRAW_DASHED:
    case DRAW_DOTTED:
    case DRAW_DOT_DASHED:
    case ERASER:
    case HIGHLIGHTER:
    case EDIT:
        drawStyle = newDrawStyle;
        break;
    }
}


void menuStructure::moveCentre(int x, int y)
{
    int maxRight = mainWindow->width - this->width()/2;
    int maxDown  = mainWindow->height - this->height()/2;

    if( x < this->width()/2 )  x = this->width()/2;
    if( x > maxRight )         x = maxRight;
    if( y < this->height()/2 ) y = this->height()/2;
    if( y > maxDown )          y = maxDown;

    move(x-this->width()/2, y-this->height()/2);
}



// Start navigation from the top again
void menuStructure::resetMenu(void)
{
    currentParentEntry.clear();
    currentMenu        = &topLevel;
    currentEntry       = 0;

    redrawRequired    = true;
}

void menuStructure::doStorageAction(storageAction_t action)
{
    switch( action )
    {
    case RECORD_SESSION:
        mainWindow->recordDiscussion();
        break;

    case PLAYBACK_RECORDED_SESSION:
        mainWindow->playbackDiscussion();
        break;

    case STORAGE_SNAPSHOT:
        mainWindow->screenSnap();
        break;

    case SNAPSHOT_GALLERY:
        qDebug() << "Snapshot gallery: TODO"; // TODO
        break;

    case STORAGE_SAVE:
        qDebug() << "Export to PDF here. Later.";
        break;
    }
}


void menuStructure::applyEditAction(pen_edit_t action)
{
    switch(action)
    {
    case EDIT_UNDO:
        mainWindow->undoPenAction(penNum);
        break;

    case EDIT_REDO:
        mainWindow->redoPenAction(penNum);
        break;

    case EDIT_DO_AGAIN:
        if( mainWindow->brushFromLast(penNum) )
        {
            // Kinda arbritrary, but feels right.
            currentMenu = NULL;
       }
       else
       {
            qDebug() << "Failed to get brush from last!";
       }
       break;

    case NO_EDIT:
        break;
    }

    mainWindow->update();
}

void menuStructure::pageAction(pageAction_t action)
{
    switch( action )
    {
    case PAGE_NEW:
        mainWindow->newPage();
        break;

    case PAGE_NEXT:
        mainWindow->nextPage();         // leave menu visible
        break;

    case PAGE_PREV:
        mainWindow->prevPage();         // leave menu visible
        break;

    case PAGE_SELECTOR:
        mainWindow->showPageSelector();
        currentMenu = NULL;             // Remove menu
        break;

    case PAGE_CLEAR:
        mainWindow->clearOverlay();
        currentMenu = NULL;             // Remove menu
        break;
    }

    qDebug() << "Page action done.";
}

void menuStructure::stickyAction(stickyAction_t action)
{
    switch( action )
    {
    case STICKY_NEW:
        mainWindow->addNewStickyStart(penNum);
        currentMenu = NULL;             // Remove menu
        break;

    case STICKY_DELETE:
        mainWindow->deleteStickyMode(penNum);
        currentMenu = NULL;             // Remove menu
        break;

    case STICKY_MOVE:
        mainWindow->startmoveofStickyNote(penNum);
        currentMenu = NULL;             // Remove menu
        break;

    case STICKY_SNAP:
    case STICKY_ARRANGE:
        qDebug() << "Stick actions: STICKY_SNAP, STICKY_ARRANGE not implemented";
        currentMenu = NULL;             // Remove menu
        break;

    }
}

void menuStructure::resizeAllIconSets(void)
{
    qDebug() << "Resize icons to:" << circleSz;

    // Re-scale current icon set (visible in ring)
    if( isVisible() && currentMenu )
    {
        for(int imgNum=0; imgNum < currentMenu->entry.size(); imgNum ++)
        {
            renderIconForCircleSize(&( currentMenu->entry[imgNum] ),circleSz);
        }
        currentMenu->circleSizeForIconRender = circleSz;

        // Rescale parent icon sets (visible in centre)
        if(  currentParentEntry.count()>0 )
        {
            for(int index=0; index<currentParentEntry.count(); index++ )
            {
                renderIconForCircleSize(currentParentEntry.at(index),circleSz);
            }
        }
    }
}

// //////////////////////
// Update with pen inputs

bool menuStructure::updateWithInput(int x, int y, buttonsActions_t *buttons)
{
    // Only do this if we are navigating the menus.

    if( currentMenu )
    {
        raise();

        // Just deal with button navigation for now. As we can have
        // multiple press events, we deal with the possibilities in
        // sequence, which means the first is the highest priority.

        if( buttons->rightMouseAction == CLICKED )
        {
            // Select the option (different actions depending on type)
            switch( currentMenu->entry[currentEntry].type )
            {
            case CHILD_MENU:
//                qDebug() << "Menu descend into " << currentMenu->entry[currentEntry].altText;

                currentParentEntry.push_back( &( currentMenu->entry[currentEntry] ) );
                currentMenu      = (menuSet_t *)(currentMenu->entry[currentEntry].data);
                currentEntry     = 0; // TODO: select value closest to current one?
                // Is it valid? If not advance till we finbd a valid one.
                if( ( (currentMenu->entry[currentEntry].validRoles & role) == 0         ||
                      0 == (currentMenu->entry[currentEntry].validRoles & mainWindow->currentSessionType()) ) )
                {
                    advanceToValidEntry(currentEntry);
                }
                redrawRequired   = true;

                break;

            case PEN_COLOUR_PRESET:
                colour      = *((QColor *)(currentMenu->entry[currentEntry].data));
                mainWindow->penColourChanged(penNum,colour);

                // Pull the current pen settings before writing to prevent
                // updates to the other fields being overwritten
                if( penNum != LOCAL_MOUSE_PEN )
                {
                    if( COS_TRUE == cos_retreive_pen_settings(penNum, &penSettings) )
                    {
                        penSettings.colour[0] = colour.red();
                        penSettings.colour[1] = colour.green();
                        penSettings.colour[2] = colour.blue();

                        cos_update_pen_settings(penNum, &penSettings);
                    }
                }

                currentMenu = NULL;
                break;

            case PEN_COLOUR_SELECTOR:
                qDebug() << "TODO: pen colour selector"; // TODO:
                currentMenu = NULL;
                break;

            case BACKGROUND_COLOUR_PRESET:
                // Function resident in overlayWindow as not per pen.
                mainWindow->paperColourChanged(penNum, *((QColor *)(currentMenu->entry[currentEntry].data)) );
                currentMenu = NULL;
                break;

            case BACKGROUND_COLOUR_SELECTOR:
                qDebug() << "TODO: background colour selector"; // TODO:
                currentMenu = NULL;
                break;

            case THICKNESS_PRESET:
                thickness = (long)(currentMenu->entry[currentEntry].data);
                mainWindow->penThicknessChanged(penNum,thickness);
                currentMenu = NULL;
                break;

            case THICKNESS_SLIDER:
                qDebug() << "TODO: pen thickness slider window"; // TODO:
                currentMenu        = NULL;
                break;

            case PEN_TYPE:
                drawStyle   = (penAction_t)((long)(currentMenu->entry[currentEntry].data));
                mainWindow->setPenType(penNum, (penAction_t)((long)(currentMenu->entry[currentEntry].data)) );
                currentMenu = NULL;
                break;

            case PEN_SHAPE:
                mainWindow->setPenShape(penNum, (shape_t)((long)(currentMenu->entry[currentEntry].data)) );
                currentMenu = NULL;
                break;

            case PEN_CONFIGURATION:
                qDebug() << "TODO: pen configuration window"; // TODO:
                currentMenu = NULL;
                break;

            case EDIT_ACTION:
                applyEditAction((pen_edit_t)((long)(currentMenu->entry[currentEntry].data)));
                break;

            case PAGE_ACTION:
                pageAction((pageAction_t)((long)(currentMenu->entry[currentEntry].data)));
                break;

            case STICKY_ACTION:
                stickyAction((stickyAction_t)((long)(currentMenu->entry[currentEntry].data)));
                currentMenu = NULL;
                break;

            case GENERIC_DIALOGUE:
                currentMenu = NULL;
                break;

            case HOST_OPTIONS:
                hostOptions(((hostOptions_t)(long)(currentMenu->entry[currentEntry].data)));
                break;

            case STORAGE_ACTION:
                doStorageAction(((storageAction_t)(long)(currentMenu->entry[currentEntry].data)));
                currentMenu = NULL;
                break;

            case NEXT_MENU_TYPE:
                switch( style )
                {
                case MINIMAL_LIST:
                    style = ROTATING_SELECTION;
                    if( currentMenu->circleSizeForIconRender != circleSz )
                    {
                        for(int imgNum=0; imgNum < currentMenu->entry.size(); imgNum ++)
                        {
                            renderIconForCircleSize(&( currentMenu->entry[imgNum] ),circleSz);
                        }

                        resizeAllIconSets();

                        currentMenu->circleSizeForIconRender = circleSz;
                    }
                    break;

                case ROTATING_SELECTION: style = MINIMAL_LIST;
                                         break;

                default:                 style = MINIMAL_LIST;
                                         break; // Repair the error
                }
                redrawRequired = true;
                repaint();
                // Leave dialogue in place
                break;

            case APPLICATION_HELP:
                currentMenu = NULL;
                break;

            case QUIT_APPLICATION:
                mainWindow->disableWhiteboard();
                mainWindow->mainMenuQuit();
                currentMenu = NULL;
                break;

            default:
                qDebug() << "Bad entryType: " << currentMenu->entry[currentEntry].type;
            }
        }

        if( buttons->leftMouseAction == CLICKED )
        {
            // pop-up a menu level. NB Top level has a parent of NULL

            if( currentParentEntry.count()>0 )
            {
                currentEntry = 0;   // This could be wrong some day, but not for current menus.
                currentParentEntry.removeLast();
            }
            currentMenu = currentMenu->parent;
            if( currentMenu != NULL )
            {
                if( ( (currentMenu->entry[currentEntry].validRoles & role) == 0         ||
                      0 == (currentMenu->entry[currentEntry].validRoles & mainWindow->currentSessionType()) ) )
                {
                    advanceToValidEntry(currentEntry);
                }
                redrawRequired = true;
                repaint();
                return true;
            }
            else
            {
                return false;
            }
        }

        if( buttons->scrollPlusAction == CLICKED )
        {
            if( buttons->centreMouse )
            {
                // Expand/contract the circular menu view
                if( circleSz<400 )
                {
                    int oldCentreX = this->x() + circleSz/2;
                    int oldCentreY = this->y() + circleSz/2;

                    circleSz = circleSz * 5 / 4;

                    resize(circleSz,circleSz);

                    resizeAllIconSets();

                    if( style == ROTATING_SELECTION )
                    {
                        moveCentre( oldCentreX, oldCentreY );
                    }
                }
            }
            else
            {
                // Flick through non valid entries (but not forever)
                advanceToValidEntry(currentEntry);
            }

            redrawRequired = true;
        }

        if( buttons->scrollMinusAction == CLICKED )
        {
            if( buttons->centreMouse )
            {
                // Expand/contract the circular menu view
                if( circleSz>30 )
                {
                    int oldCentreX = this->x() + circleSz/2;
                    int oldCentreY = this->y() + circleSz/2;

                    circleSz = circleSz * 4 / 5;

                    resize(circleSz,circleSz);

                    resizeAllIconSets();

                    if( style == ROTATING_SELECTION )
                    {
                        moveCentre( oldCentreX, oldCentreY );
                    }
                }
            }
            else
            {
                // Flick through non valid entries (but not forever)
                int prev = currentEntry;

                do
                {
                    if( prev > 0 )
                    {
                        prev --;
                    }
                    else
                    {
                        prev = currentMenu->entry.size() - 1;
                    }

                } while(prev != currentEntry                                    &&
                        ( (currentMenu->entry[prev].validRoles & role) == 0     ||
                           0 == (currentMenu->entry[prev].validRoles & mainWindow->currentSessionType()) ) );

                currentEntry = prev;
            }

            redrawRequired = true;
        }

    } // Currently in a menu

    if( currentMenu == NULL )
    {
        qDebug() << "Hide menu as currentMenu == NULL";

        hide();
    }
    else
    {
        update();
    }

    return currentMenu != NULL;
}


void menuStructure::advanceToValidEntry(int &entryNum)
{
    // Okay, so we need to advance until we find a valid entry.

    int next = entryNum;

    do
    {
        if( next < currentMenu->entry.size()-1 )
        {
            next ++;
        }
        else
        {
            next = 0;
        }

    } while(next != entryNum                                            &&
            ( (currentMenu->entry[next].validRoles & role) == 0         ||
              0 == (currentMenu->entry[next].validRoles & mainWindow->currentSessionType()) ) );

    if( entryNum == next )
    {
        qDebug() << "menuStructure::advanceToValidEntry() Failed to find a valid entry.";
    }

    entryNum = next;
}


void menuStructure::drawMinimalList(void)
{
    int htPerOption = 18;

    qDebug() << "Redraw minimal list.";

    int numMenuOptions = 0;
    for(int opt=0; opt<currentMenu->entry.size(); opt++)
    {
        // Advance opt until it is a valid entry for us
        if( 0 != (currentMenu->entry[opt].validRoles & role)                           &&
            0 != (currentMenu->entry[opt].validRoles & mainWindow->currentSessionType()) )
        {
            numMenuOptions ++;
        }
    }

    // Calculate the size
    int w = 110;
    int h = htPerOption*numMenuOptions;

    // Might as well ensure the window is the same size...
    resize(w+12, h+12);

    // And get an image
    int currentWidth  = w + 12;
    int currentHeight = h + 12;
    menuImage = QImage(currentWidth,currentHeight, QImage::Format_ARGB32_Premultiplied);

    // And paint in the image
    menuImage.fill(Qt::transparent);

    // A nice white blob to write on
    QPainter paint(&menuImage);
    paint.setRenderHint(QPainter::Antialiasing);
    paint.setBrush(Qt::white);
    paint.drawRoundedRect(QRectF(1,1, w+12-1,h+12-1), 20.0, 15.0, Qt::RelativeSize);

    // Add the text
    paint.setRenderHint(QPainter::Antialiasing);
    paint.setBrush(Qt::black);
    int textTop = 0;
    int opt     = 0;
    for(int c=0; c<numMenuOptions; c++)
    {
        // Advance opt until it is a valid entry for us
        while( 0 == (currentMenu->entry[opt].validRoles & role)                           ||
               0 == (currentMenu->entry[opt].validRoles & mainWindow->currentSessionType()) )
        {
            opt ++;

            if( opt>=currentMenu->entry.size() )
            {
                qDebug() << "drawMinimalList() Internal error: reduced menu count mismatch.";
                opt = 0;
                break;
            }
        }

        if( opt == currentEntry )
        {
            // And give the current option a background colour
            paint.setBrush(Qt::gray);
            paint.setPen(Qt::gray);
            paint.drawRoundedRect(QRectF(6,5+htPerOption*c, w-1,htPerOption+3-1),
                                  20.0, 15.0, Qt::RelativeSize);
        }

        textTop = 8+htPerOption*c;

        // Grey out invalid options
//        if(currentMenu->entry[opt].validRoles & role) paint.setPen(Qt::black);
//        else                                    paint.setPen(Qt::gray);
        paint.setPen(Qt::black);    // Now only showing valid optionsto save space

        paint.drawText(QRect(6,textTop, w, htPerOption),Qt::AlignHCenter,
                       currentMenu->entry[opt].altText);

        opt ++;
    }

    paint.setPen(colour);
    paint.drawLine(QPoint(20, textTop+htPerOption-2),QPoint(w-8,textTop+htPerOption-2));
    paint.drawLine(QPoint(20, 6),QPoint(w-8,6));
    paint.end();

    redrawRequired = false;
}

void menuStructure::renderIconForCircleSize(menuEntry_t *entry, int size)
{
    QSvgRenderer re;
    int          iconSize = size/5;

    if( entry->icon != NULL ) delete(entry->icon);

    entry->icon = new QImage(iconSize,iconSize, QImage::Format_ARGB32_Premultiplied);
    entry->icon->fill(Qt::transparent);

    QPainter paint(entry->icon);
    paint.setRenderHint(QPainter::Antialiasing);

    if( entry->icon_resource.length() > 1 )
    {
        // Render the SVG icon into this image
        re.load(entry->icon_resource);
        re.render(&paint);

//        qDebug() << "Render resource: " << entry->icon_resource;
    }
    else if( entry->type == PEN_COLOUR_PRESET       ||
             entry->type == BACKGROUND_COLOUR_PRESET  )
    {
        // Render a colour swatch
        paint.setBrush( *((QColor *)(entry->data)) );
        paint.setPen( QPen(Qt::darkGray,3) );
        paint.drawEllipse(0,0, iconSize,iconSize );

//        qDebug() << "Render icon swatch: " << *((QColor *)(entry->data));
    }
    else
    {
        // Render the alt-text for now ? paint.drawText(int x, int y, const QString & text)
    }

    paint.end();
}


void menuStructure::drawRotatingSelection(void)
{
    if( currentMenu->circleSizeForIconRender != circleSz )
    {
        for(int imgNum=0; imgNum < currentMenu->entry.size(); imgNum ++)
        {
            renderIconForCircleSize(&( currentMenu->entry[imgNum] ),circleSz);
        }

        currentMenu->circleSizeForIconRender = circleSz;
    }

    resize(circleSz,circleSz);

    menuImage = QImage(circleSz,circleSz, QImage::Format_ARGB32_Premultiplied);
    menuImage.fill(Qt::transparent);

    // Angle calculations. NB In Qt each angle int is 1/16 of a degree

    int numSegments = 0;
    for(int seg=0; seg<currentMenu->entry.size(); seg++)
    {
        // Advance opt until it is a valid entry for us
        if( 0 != (currentMenu->entry[seg].validRoles & role)                           &&
            0 != (currentMenu->entry[seg].validRoles & mainWindow->currentSessionType()) )
        {
            numSegments ++;
        }
    }

    int segmentAngleWidth = (numSegments != 0) ? 360*16 / numSegments : 0;

    // Outer ring: dark gray; icon ring white (& one gray); inner circle dark gray

    QPainter paint(&menuImage);
    paint.setRenderHint(QPainter::Antialiasing);

    // outermost ring (current pen colour)
    paint.setPen(colour);
    paint.setBrush(colour);
    paint.drawEllipse(QPoint(circleSz/2,circleSz/2),circleSz/2-circleSz/30,circleSz/2-circleSz/30);

    // middle ring that contains the icons
    paint.setPen(Qt::white);
    paint.setBrush(Qt::white);
    paint.drawEllipse(QPoint(circleSz/2,circleSz/2),circleSz/2-circleSz/15,circleSz/2-circleSz/15);

    // Draw options
    int    drawX1, drawY1, drawX2, drawY2;
    QPen   pen;
    pen.setColor(Qt::transparent);
    pen.setWidth(2);
    paint.setPen(pen);

    int opt = 0;
    for(int seg=0; seg<numSegments; seg++)
    {
        // Advance opt until it is a valid entry for us
        while( 0 == (currentMenu->entry[opt].validRoles & role)                           ||
               0 == (currentMenu->entry[opt].validRoles & mainWindow->currentSessionType()) )
        {
            opt ++;

            if( opt>=currentMenu->entry.size() )
            {
                qDebug() << "drawRotatingSelection() Internal error: reduced menu count mismatch.";
                opt = 0;
                break;
            }
        }

        if( opt == currentEntry )
        {
            int selectionAngleStart =  90*16 - (seg * segmentAngleWidth);

            // Draw highlight (gray) as a pie chart segment
            paint.setPen(Qt::lightGray);
            paint.setBrush(Qt::lightGray);
            paint.drawPie(QRect(circleSz/15,circleSz/15,circleSz-circleSz*2/15,circleSz-circleSz*2/15),
                          selectionAngleStart, -segmentAngleWidth);
        }

        // add the icon
        if( currentMenu->entry[opt].icon )
        {
            double angleCenterRad = (90*16 - (segmentAngleWidth*seg + segmentAngleWidth/2)) * 2*PI / (16*360);
            QImage img            = *(currentMenu->entry[opt].icon);
            int    drawX1         = circleSz/2 + (cos(angleCenterRad)*circleSz*0.5*0.63) - img.width()/2;
            int    drawY1         = circleSz/2 - (sin(angleCenterRad)*circleSz*0.5*0.63) - img.height()/2;

            // Fade invalid options
//            if( currentMenu->entry[opt].validRoles & role )
//            {
//                paint.setOpacity(1.0);
//            }
//            else
//            {
//                paint.setOpacity(0.4);
//            }
            paint.setOpacity(1.0); // Only draw valid options to save space

            paint.drawImage(drawX1, drawY1, *(currentMenu->entry[opt].icon));
        }

        // Do the ticks
        double angleBoundaryRad = (90*16 - (segmentAngleWidth*seg)) * 2*PI / (16*360);
        drawX1 = circleSz/2 + (cos(angleBoundaryRad)*circleSz*0.5*0.80);
        drawY1 = circleSz/2 - (sin(angleBoundaryRad)*circleSz*0.5*0.80);
        drawX2 = circleSz/2 + (cos(angleBoundaryRad)*circleSz*0.5*(1.0-1.0/15.0));
        drawY2 = circleSz/2 - (sin(angleBoundaryRad)*circleSz*0.5*(1.0-1.0/15.0));

        paint.setPen(Qt::lightGray);
        paint.drawLine(drawX1,drawY1, drawX2,drawY2);

        opt ++;
    }

    // Fill the core
    paint.setPen(Qt::darkGray);
    paint.setBrush(Qt::darkGray);
    paint.drawEllipse(QPoint(circleSz/2,circleSz/2),circleSz/5,circleSz/5);

    // Draw parent icon, if present
    if( currentParentEntry.count() > 0 )
    {
        QImage img = *(currentParentEntry.last()->icon);
        paint.drawImage((circleSz-img.width())/2,(circleSz-img.height())/2,img);
    }

    paint.end();

    redrawRequired = false;
}


void menuStructure::paintEvent(QPaintEvent *pev)
{
    if( ! currentMenu )
    {
#if 0
        static int debugCount = 0;

        debugCount ++;

        if( debugCount>400 )
        {
            qDebug() << "400x Paint menuStructure event, but currentMenu not set!";

            debugCount = 0;
        }
#endif
        return;
    }

    QPainter painter(this);

    if( redrawRequired )
    {
        // call the appropriate functions to generate an image of the current state
        switch( style )
        {
        case MINIMAL_LIST:

            drawMinimalList();
            break;

        case ROTATING_SELECTION:

            drawRotatingSelection();
            break;
        }
    }

    // Add the overlay
    painter.drawImage(0,0, menuImage);

    painter.end();

    // Allow for changing the menu requiring the menu to be moved away from the screen edges

    int maxRight = mainWindow->width - this->width();
    int maxDown  = mainWindow->height - this->height();

    int x = this->x();
    int y = this->y();

    if( x < 0 )          x = 0;
    if( x > maxRight )   x = maxRight;
    if( y < 0 )          y = 0;
    if( y > maxDown )    y = maxDown;

    move(x,y);
}

