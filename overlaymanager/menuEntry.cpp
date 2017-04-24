#include "menuEntry.h"

#include <vector>

#include <QPainter>

// ////////////////////
// Construction methods
// ////////////////////

menuEntry::menuEntry(bool eventOnEntry, int optParam)
{
    optionalParameter = optParam;

    name.clear();
    cachedImage.clear();

    eventWhileSelecting = eventOnEntry;

    numOptions        = 0;
    currentOption     = 0;
}

int menuEntry::addOption(QString text, int newValue)
{
    name.push_back(text);
    value.push_back(newValue);

    QImage *img = new QImage(100,18, QImage::Format_ARGB32_Premultiplied);

    // And paint in the image (black text on a transparent background)
    img->fill(Qt::transparent);

    QPainter paint(img);
    paint.setRenderHint(QPainter::Antialiasing);
    paint.setBrush(Qt::black);
    paint.drawText(QRect(0, 0, img->width(), img->height()), Qt::AlignHCenter, text);
    paint.end();

    cachedImage.push_back(img);
    numOptions ++;

    return numOptions-1;
}

void menuEntry::alterOption(int id, QString text, int newValue)
{
    name[id]  = text;
    value[id] = newValue;

    // And paint in the image (black text on a transparent background)
    cachedImage[id]->fill(Qt::transparent);

    QPainter paint(cachedImage[id]);
    paint.setRenderHint(QPainter::Antialiasing);
    paint.setBrush(Qt::black);
    paint.drawText(QRect(0, 0, cachedImage[id]->width(), cachedImage[id]->height()), Qt::AlignHCenter, text);
    paint.end();
}

void menuEntry::selectOption(int valueIndex)
{
    // Sanify check
    if( valueIndex<0 || valueIndex>=value.size() ) return;

    currentOption   = valueIndex;
    selectingOption = valueIndex;
}


// //////////
// And update
// //////////

void menuEntry::applyAction(click_type_t action)
{
    switch(action)
    {
    case optionsClick:
    case selectClick:

        emit entryChanged(value[currentOption], optionalParameter);

        break;

    case nextEntry:
        if( currentOption>=(numOptions-1) ) currentOption = 0;
        else                                currentOption ++;

        emit entryViewUpdateRequired(optionalParameter);

        if( eventWhileSelecting && numOptions>0 )
        {
            emit entryChanged(value[currentOption], optionalParameter);
        }
        break;

    case previousEntry:
        if( currentOption==0 ) currentOption = numOptions - 1;
        else                   currentOption --;

        emit entryViewUpdateRequired(optionalParameter);

        if( eventWhileSelecting && numOptions>0 )
        {
            emit entryChanged(value[currentOption], optionalParameter);
        }
        break;

    case noEvent:
        break;
    }

    return;
}


// ///////////////////
// Return data methods
// ///////////////////

bool menuEntry::haveMultipleOptions(void)
{
    return numOptions>1;
}

QImage *menuEntry::getCurrentOptionImage(void)
{
    if( numOptions < 1 ) throw string("Bad option menu access. No options set yet.");

    return cachedImage[currentOption];
}

QImage *menuEntry::getNextOptionImage(void)
{
    if( numOptions < 1 ) throw string("Bad option menu access. No options set yet.");

    if( (currentOption+1) >= numOptions ) return cachedImage[0];
    else                                  return cachedImage[currentOption+1];
}

QImage *menuEntry::getPrevOptionImage(void)
{
    if( numOptions < 1 ) throw string("Bad option menu access. No options set yet.");

    if( (currentOption-1) < 0 ) return cachedImage[numOptions-1];
    else                        return cachedImage[currentOption-1];
}


