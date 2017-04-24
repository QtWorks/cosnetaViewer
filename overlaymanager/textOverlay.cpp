#include "textOverlay.h"

#include <QImage>
#include <QPainter>

#include <QPainter>
#include <QDebug>


#define MIN_FONT_HEIGHT 8   /* Probably can't be read any smaller than this */

static QFont getFixedPitchFont(void)
{
    QFont font("monospace");

    if( font.fixedPitch() ) return font;
    font.setFixedPitch(true);
    if( font.fixedPitch() ) return font;
    font.setFamily("courier");
    if( font.fixedPitch() ) return font;

    return font;
}


textOverlay::textOverlay(int width, int height)
{
    drawImage = QImage(width, height, QImage::Format_ARGB32_Premultiplied);
    redrawImage(0,0, width,height);

    lastFontHeight = 24;    // Pixels

    font = getFixedPitchFont();
}

void textOverlay::resize(int width, int height)
{
    drawImage = QImage(width, height, QImage::Format_ARGB32_Premultiplied);
    redrawImage(0,0, width,height);
}

textOverlay::~textOverlay()
{
    // delete the text layers
    textLayer.clear();
}


void textOverlay::addCharacter(QChar character, int pixelX, int pixelY, int sz, QColor colour)
{
    if( sz == 0 ) sz = lastFontHeight;

    // snap size to nearest allowed (always next up from the requested size)
    int snappedSize = MIN_FONT_HEIGHT;

    while( snappedSize < sz ) snappedSize = snappedSize * 3; // Trial and error here...

    // Get a reference to the correct plane. If it doesn't exist, create a new text plane for it
    sizedTextLayer *layer = getTextLayer(snappedSize);

    // And the character coordinates for it
    int chX = pixelX / layer->fontCharWidthPx;
    int chY = pixelY / layer->fontCharHeightPx;

    lastFontHeight = layer->fontCharHeightPx;

    qDebug() << "Ht" << sz << "Row,col=" << chX << chY;

    // Check all planes for overlap and erase the characters under the new one
    // Also update a 'disturbed area' so we don't need a full redraw - larger than new ch
    int  disturbedX = chX * layer->fontCharWidthPx;
    int  disturbedY = chY * layer->fontCharHeightPx;
    int  disturbedHeight = layer->fontCharHeightPx;
    int  disturbedWidth  = layer->fontCharWidthPx;

    eraseRectFromAllPlanes(disturbedX,disturbedY, disturbedWidth,disturbedHeight);

     // Dynamic allocation
    QVector<QColor> colourRow;
    while( layer->textLine.size()   <= chY )    layer->textLine.push_back(QString(""));
    while( layer->charColour.size() <= chY )    layer->charColour.push_back(colourRow);

    while( layer->textLine[chY].size()   <= chX ) layer->textLine[chY].append(QChar::Space);
    while( layer->charColour[chY].size() <= chX ) layer->charColour[chY].push_back(Qt::black);

    // add the new character, updating the disturbed area
    layer->textLine[chY].replace(chX,1,character);
    layer->charColour[chY][chX] = colour;

    qDebug() << "Line" << chY << layer->textLine[chY];

    // redraw
    redrawImage( disturbedX,disturbedY, disturbedHeight,disturbedWidth );
}


void textOverlay::redrawImage(int x, int y, int width, int height)
{
    drawImage.fill(Qt::transparent);

    QPainter paint(&drawImage);

    // Go through each layer and draw the characters onto the image
    for(int layer=0; layer<textLayer.size(); layer++)
    {
        font.setPixelSize(textLayer[layer].fontCharHeightPx);

        paint.setFont(font);

        for(int line=0; line<textLayer[layer].textLine.size(); line++ )
        {
            int yPos = (line+1)*textLayer[layer].fontCharHeightPx; // +1 as baseline of font

            for(int x=0; x<textLayer[layer].textLine[line].size(); x++)
            {
                QString c = QString(textLayer[layer].textLine[line][x]);

                if( c != QString(" "))
                {
                    paint.setPen(textLayer[layer].charColour[line][x]);
                    paint.drawText( QPoint(x*textLayer[layer].fontCharWidthPx,yPos),c );
                }
            }
        }
    }
}


// Allow our parent to draw the overlay as an image
void textOverlay::paintImage(QPainter *painter)
{
    painter->drawImage(0,0, drawImage);
}


// Either return a reference to the existing layer, or create
// a new layer and return a reference.
sizedTextLayer *textOverlay::getTextLayer(int fontHeight)
{
    // Look for existing layer
    for( int layer=0; layer<textLayer.size(); layer++ )
    {
        if( textLayer[layer].fontCharHeightPx == fontHeight )
        {
            return &( textLayer[layer] );
        }
    }

    qDebug() << "Create a new text layer for text of height" << fontHeight;

    // Not found, so create as a new layer
    sizedTextLayer newLayer;

    font.setPixelSize(fontHeight);

    newLayer.fontCharHeightPx = fontHeight;
    newLayer.fontCharWidthPx  = QFontMetrics(font).width(QChar::fromLatin1('m'));

    textLayer.push_back(newLayer);

    return &( textLayer.last() );
}

// Go through all the planes and erase (set to space) any that enter this rect.
// NB Rect is updated to match quantisation on all scales
void textOverlay::eraseRectFromAllPlanes(int &inX, int &inY, int &inWidth, int &inHeight)
{
    int x = inX;
    int y = inY;
    int width  = inWidth;
    int height = inHeight;

    for( int layer=0; layer<textLayer.size(); layer++ )
    {
        qDebug() << "Layer Ht:" << textLayer[layer].fontCharHeightPx <<
                    "erase a character (write ' ' to all text layer in same place";

        int startCol = inX / textLayer[layer].fontCharWidthPx;
        int startRow = inY / textLayer[layer].fontCharHeightPx;
        int endCol = 1+(inX + inWidth)  / textLayer[layer].fontCharWidthPx;
        int endRow = 1+(inY + inHeight) / textLayer[layer].fontCharHeightPx;

        for(int line=startRow; line<endRow && line<textLayer[layer].textLine.size(); line++)
        {
            for( int chNum=startCol; chNum<endCol &&
                                     chNum<textLayer[layer].textLine[line].size(); chNum++)
            {
                textLayer[layer].textLine[line].replace(chNum,1,QChar::Space);
            }
        }
    }

    inX = x;
    inY = y;
    inWidth  = width;
    inHeight = height;
}


