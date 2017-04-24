#include "dropMenu.h"

#include <QWidget>
#include <QPainter>


#include <iostream>

dropMenu::dropMenu(QWidget *parent) :
                   QWidget(parent, Qt::FramelessWindowHint|Qt::WindowStaysOnTopHint)
{
    setAttribute(Qt::WA_TranslucentBackground);

    option.clear();

    scnWidth  = 1024; // Hopefully never used, but small enough to be noticable as a bug
    scnHeight = 768;

    menuImage = NULL;
}

void dropMenu::setScreenSize(int w, int h)
{
    scnWidth  = w;
    scnHeight = h;
}

int dropMenu::addOption(menuEntry *entry)
{
    // Add a new option. The current intent is to have 6 entries per option (think of
    // a nut on a bolt) but that isn't fixed here.
    option.push_back(entry);

    // Return the new total
    return ((int)option.size())-1;
}


void dropMenu::setColour(QColor newColour)
{
    colour = newColour;
}


// Build a menu image from scratch. This will be updated in parts while in use
void dropMenu::cacheImage(void)
{
    int htPerOption = 18;

    if( menuImage != NULL ) delete menuImage;

    int numMenuOptions = (int)option.size();

    // Calculate the size
    int w = 100;
    int h = htPerOption*numMenuOptions;

    // Might as well ensure the window is the same size...
    resize(w+12, h+12);

    // And get an image
    currentWidth  = w + 12;
    currentHeight = h + 12;
    menuImage = new QImage(currentWidth,currentHeight, QImage::Format_ARGB32_Premultiplied);

    // And paint in the image
    menuImage->fill(Qt::transparent);

    QPainter paint(menuImage);
    paint.setRenderHint(QPainter::Antialiasing);
    paint.setBrush(Qt::white);
    paint.drawRoundedRect(QRectF(1, 1,w+12-1, h+12-1),
                            20.0, 15.0, Qt::RelativeSize);

    paint.setPen(Qt::black);
    int textTop = 0;
    for(int opt=0; opt<numMenuOptions; opt++)
    {
        textTop = 8+htPerOption*opt;

        paint.drawImage(6,textTop,*(option[opt]->getCurrentOptionImage()));

//        paint.drawText(QRect(6, textTop, w-6, htPerOption),
//                       Qt::AlignHCenter,      option[opt]->entryName[0]);
//        paint.drawLine(QPoint(20, textTop-(htPerOption/4)),QPoint(w-8,textTop-(htPerOption/4)));
    }

    paint.setPen(colour);
    paint.drawLine(QPoint(20, textTop+htPerOption-2),QPoint(w-8,textTop+htPerOption-2));
    paint.drawLine(QPoint(20, 6),QPoint(w-8,6));
    paint.end();
}


void dropMenu::paintEvent(QPaintEvent *)
{
    QPainter painter(this);

    // Make sure we have an image
    if( menuImage == NULL ) cacheImage();

    // Add the overlay
    painter.drawImage(0,0, *menuImage);

    painter.end();
}


// Pointer clicked and do any updates. If the return value is true, then
// the menu is now visible. If false, the menu is not visible.

bool dropMenu::pointerAndclickMenu(int xLoc, int yLoc, click_type_t type)
{
    bool penIsOverhead = false;
    int  optionNum = 0;

    // Check whether the pen is overhead and get option number
    if( isVisible() )
    {
        if( xLoc > this->x() && xLoc<(this->x()+currentWidth) &&
            yLoc > this->y() && yLoc<(this->y()+currentHeight)    )
        {
            penIsOverhead = true;

            optionNum = (yLoc - 6 - this->y())/18;

            if( optionNum<0 )                   optionNum = 0;
            else if( optionNum>=option.size() ) optionNum = (int)(option.size()) - 1;
        }
    }

    switch( type )
    {
    case optionsClick:  if( isVisible() )
                        {
                            if( penIsOverhead ) // Menu button pressed on a selection
                            {
                                // Only hidden if caller thinks it's suitable
                                option[optionNum]->applyAction(type);
                            }
                            else    // Always hide() for off menu click
                            {
                                hide();
                            }
                            return true;
                        }
                        else  // Menu not visible & menu pressed: make it appear
                        {
                            // Menu adds to coords, so only check maxima
                            if( (xLoc+currentWidth)>scnWidth )   xLoc = (scnWidth  - currentWidth);
                            if( (yLoc+currentHeight)>scnHeight ) yLoc = (scnHeight - currentHeight);
                            move(xLoc,yLoc);
                            show();
                            return true;
                        }
                        break;

    case selectClick:   if( isVisible() )
                        {
                            if( penIsOverhead )  // Can select menu option with left click
                            {
                                option[optionNum]->applyAction(type);
                            }
                            else
                            {
                                hide();
                            }
                            return true;
                        }
                        break;

    case nextEntry:
    case previousEntry: if( penIsOverhead )
                        {
                            option[optionNum]->applyAction(type);
                            return true;
                        }
                        break;

    default:            break;
    }

    return false;
}

