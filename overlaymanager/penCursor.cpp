#include <QPainter>
#include <QSvgRenderer>
#include <QDebug>
#include <QRadialGradient>

#include "penCursor.h"

#define MINIMUM_CURSOR_SIZE 2

// This object is mainly just a window with no border with a fixed
// shape with selectable colour. The position and visibility is
// inherited behaviour.

penCursor::penCursor(QColor colour, QWidget *parent) :
               QWidget(parent, /*Qt::FramelessWindowHint        |*/
                               Qt::X11BypassWindowManagerHint |
                               Qt::WindowStaysOnTopHint        )
{
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_TransparentForMouseEvents,true); // Pass mouse events through

    penSize          = 8;
    outerSize        = 15 + penSize;
    penColour        = Qt::magenta;
    type             = CURS_NORMAL;
    beforeStickyType = type;
    opaqueness       = 1.0;
    moveImageRes     = ":/stickies/images/stickies/move.svg";
    textImageRes     = ":/nib/images/nib/text.svg";

    presentationLaserIsOn = false;
    stickyWidth           = outerSize;
    stickyHeight          = outerSize;
    presentationBlobSize  = 80;
    penMode               = MODE_OVERLAY;

    rebuildImage();

    updatePenColour(colour);
    resize(15,15);

    hide(); // Must be explicitly shown
}

penCursor::~penCursor(void)
{
    if( isVisible() )
    {
        hide();
    }
}


void penCursor::paintEvent(QPaintEvent *)
{
    QPainter painter(this);

    // Add the overlay
    painter.drawImage(0,0, cursorImage);

    painter.end();
}




// Note that we assume a square window here.
void penCursor::updatePenColour(QColor colour)
{
    if( colour != penColour )
    {
        penColour = colour;

        rebuildImage();
    }
}

QColor penCursor::getCurrentColour(void)
{
    return penColour;
}


// Note that we assume a square window here.
void penCursor::updatePenSize(int newSize)
{
    if( newSize != penSize && type != CURS_TEXT )
    {
        penSize   = newSize;
        outerSize = 15+penSize;

        rebuildImage();
    }
}

int penCursor::getCurrentPenSize(void)
{
    return penSize;
}

void penCursor::setCursorFromBrush(QList<QPoint> *brush)
{
    QPoint  initialCenter = QPoint(this->x()+(this->width()/2),this->x()+(this->height()/2));

    opaqueness = 1.0;

    brushPoints.clear();

    // Copy, centered on origin & record width & height. Also this ensures we have a "deep copy"
    int xMax, xMin, yMax, yMin;

    xMax = (*brush).at(0).x();
    yMax = (*brush).at(0).y();
    xMin = xMax;
    yMin = yMax;

    for(int pt=1; pt<(*brush).size(); pt++)
    {
        if( (*brush).at(pt).x()>xMax ) xMax = (*brush).at(pt).x();
        if( (*brush).at(pt).x()<xMin ) xMin = (*brush).at(pt).x();

        if( (*brush).at(pt).y()>yMax ) yMax = (*brush).at(pt).y();
        if( (*brush).at(pt).y()<yMin ) yMin = (*brush).at(pt).y();
    }

    QPoint brushTopLeft = QPoint(xMin, yMin);

    brushWidth  = xMax - xMin;
    brushHeight = yMax - yMin;

    for(int pt=0; pt<(*brush).size(); pt++)
    {
        brushPoints.append((*brush).at(pt)-brushTopLeft);
    }

    // Set the type
    type = CURS_BRUSH;

    // And regenerate the current brush image
    rebuildBrushCursor();

    // Keep the cursor centered on the same point
    moveCentre( initialCenter.x(),initialCenter.y() );

    // Refresh the visible view
    update();
}


void penCursor::setStickyAddBrush(int size, QColor colour)
{
    QPoint  initialCenter = QPoint(this->x()+(this->width()/2),this->x()+(this->height()/2));

    opaqueness = 0.4f;

    stickyWidth  = size;
    stickyHeight = size;
    filledColour = colour;

    // Set the type (which will cause it to be filled)
    if( type != CURS_STICKY )
    {
        beforeStickyType = type;
        type             = CURS_STICKY;
    }

    // And regenerate the current brush image
    resize(size,size);

    qDebug() << "setStickyAddBrush() w/h:" << size << size;

    // NB No delete due to implicit data sharing
    cursorImage = QImage(size,size, QImage::Format_ARGB32_Premultiplied);

    cursorImage.fill(Qt::transparent);

    rebuildStickyCursor();

    // Keep the cursor centered on the same point
    moveCentre( initialCenter.x(),initialCenter.y() );

    // Refresh the visible view
    update();
}


void penCursor::removeStickyAddBrush(void)
{
    qDebug() << "removeStickyAddBrush: revert type to" << beforeStickyType;

    type = beforeStickyType;

    // And regenerate the current brush image
    rebuildImage();

    // Refresh the visible view
    update();
}


void penCursor::setDeleteBrush(void)
{
    type    = CURS_DELETE;
    penMode = MODE_OVERLAY;

    rebuildDeleteCursor();

    update();
}


void penCursor::setMoveBrush(void)
{
    type    = CURS_MOVE;
    penMode = MODE_OVERLAY;

    rebuildMoveCursor();

    update();
}


void penCursor::setTextBrush(void)
{
    type    = CURS_TEXT;
    penMode = MODE_OVERLAY;

    rebuildTextCursor();

    update();
}


void penCursor::setdefaultCursor(void)
{
    if( brushPoints.size() > 0 )
    {
        brushPoints.clear();
    }

    rebuildStandardCursor();

    type    = CURS_NORMAL;
    penMode = MODE_OVERLAY;

    // Refresh the visible view
    update();
}


void penCursor::moveCentre(int x, int y)
{
    move(x-width()/2, y-height()/2);
}

void penCursor::rebuildStandardCursor(void)
{
    opaqueness = 1.0;

    resize(outerSize,outerSize);

    // NB No delete due to implicit data sharing
    cursorImage = QImage(outerSize,outerSize, QImage::Format_ARGB32_Premultiplied);

    int    halfSize = outerSize/2;

    QColor innerColour;
    QColor outerColour;

    if( penColour.value() < 128 ) // darker than 0.5
    {
        innerColour = penColour.lighter();
        outerColour = penColour;
    }
    else                    // brighter than 0.5
    {
        innerColour = penColour;
        outerColour = penColour.darker();
    }

    cursorImage.fill(Qt::transparent);

    QPainter painter(&cursorImage);

    painter.setRenderHint(QPainter::Antialiasing);

    painter.setPen(QPen(innerColour, 1.5));
    painter.drawEllipse(QPoint(halfSize,halfSize), halfSize-4, halfSize-4);
    painter.setPen(QPen(outerColour, 1.5));
    painter.drawEllipse(QPoint(halfSize,halfSize), halfSize-2, halfSize-2);

    if( halfSize>16 )
    {
        painter.setPen(QPen(outerColour,2.0));
        painter.drawLine(halfSize-4,halfSize+0,halfSize+4,halfSize+0);
        painter.drawLine(halfSize+0,halfSize-4,halfSize+0,halfSize+4);
        painter.setPen(QPen(innerColour,1.0));
        painter.drawLine(halfSize-4,halfSize+0,halfSize+4,halfSize+0);
        painter.drawLine(halfSize+0,halfSize-4,halfSize+0,halfSize+4);
    }

    painter.end();
}


void penCursor::rebuildPresentationCursor(void)
{
    qDebug() << "rebuildPresentationCursor: size =" << presentationBlobSize;
    opaqueness = 1.0;

    resize(presentationBlobSize,presentationBlobSize);

    // NB No delete due to implicit data sharing
    cursorImage = QImage(presentationBlobSize,presentationBlobSize, QImage::Format_ARGB32_Premultiplied);

    int    halfSize = presentationBlobSize/2;

    cursorImage.fill(Qt::transparent);

    QPainter painter(&cursorImage);

    painter.setRenderHint(QPainter::Antialiasing);

    QColor outsideColour = penColour;
    outsideColour.setAlpha(0);

    QRadialGradient gradient(halfSize,halfSize,halfSize);
    gradient.setColorAt(0.0,outsideColour);
    gradient.setColorAt(0.4,outsideColour);
    gradient.setColorAt(0.7,penColour);
    gradient.setColorAt(1.0,outsideColour);

    painter.setPen(Qt::transparent);
    painter.setBrush(QBrush(gradient));
    painter.drawEllipse(QPoint(halfSize,halfSize), halfSize-4, halfSize-4);

    painter.end();
}


void penCursor::rebuildDeleteCursor(void)
{
    resize(outerSize,outerSize);

    qDebug() << "rebuildDeleteCursor() w/h:" << outerSize << outerSize << "pts:" << brushPoints.size();

    // NB No delete due to implicit data sharing
    cursorImage = QImage(outerSize,outerSize, QImage::Format_ARGB32_Premultiplied);

    cursorImage.fill(Qt::transparent);

    QPainter painter(&cursorImage);

    QPen drawPen;
    drawPen.setColor(Qt::red);
    drawPen.setWidth(4);

    painter.setPen(drawPen);

    painter.drawLine(QPoint(outerSize*0.25,outerSize*0.25), QPoint(outerSize*0.75,outerSize*0.75));
    painter.drawLine(QPoint(outerSize*0.75,outerSize*0.25), QPoint(outerSize*0.25,outerSize*0.75));

    painter.end();
}


void penCursor::rebuildMoveCursor(void)
{
    resize(outerSize,outerSize);

    qDebug() << "rebuildMoveCursor() w/h:" << outerSize << outerSize << "pts:" << brushPoints.size();

    // NB No delete due to implicit data sharing
    cursorImage = QImage(outerSize,outerSize, QImage::Format_ARGB32_Premultiplied);

    cursorImage.fill(Qt::transparent);

    QSvgRenderer renderer;

    QPainter painter(&cursorImage);

    renderer.load(moveImageRes);
    renderer.render(&painter);
}


void penCursor::rebuildTextCursor(void)
{
    resize(32,32);

    qDebug() << "rebuildTextCursor() w/h: 32x32 pixels.";

    // NB No delete due to implicit data sharing
    cursorImage = QImage(32,32, QImage::Format_ARGB32_Premultiplied);

    cursorImage.fill(Qt::transparent);

    QSvgRenderer renderer;

    QPainter painter(&cursorImage);

    renderer.load(textImageRes);
    renderer.render(&painter);
}


void penCursor::rebuildBrushCursor(void)
{
    resize(brushWidth,brushHeight);

    qDebug() << "rebuildBrushCursor() w/h:" << brushWidth << brushHeight << "pts:" << brushPoints.size();

    // NB No delete due to implicit data sharing
    cursorImage = QImage(brushWidth,brushHeight, QImage::Format_ARGB32_Premultiplied);

    cursorImage.fill(Qt::transparent);

    QPainter painter(&cursorImage);

    painter.setOpacity(opaqueness);

    QPen drawPen;
    drawPen.setColor(penColour);
    drawPen.setWidth(2);
//    drawPen.setStyle(Qt::DotLine);
    painter.setPen(drawPen);

    painter.setRenderHint(QPainter::Antialiasing);

    for(int pt=1; pt<brushPoints.size(); pt++ )
    {
        painter.drawLine(brushPoints[pt-1],brushPoints[pt]);
    }

    painter.end();
}


void penCursor::rebuildStickyCursor(void)
{
    resize(stickyWidth,stickyWidth);

    qDebug() << "rebuildStickyCursor() w/h:" << brushWidth << brushHeight << "pts:" << brushPoints.size();

    // NB No delete due to implicit data sharing
    cursorImage = QImage(stickyWidth,stickyWidth, QImage::Format_ARGB32_Premultiplied);

    cursorImage.fill(Qt::transparent);

    QPainter painter(&cursorImage);

    painter.setOpacity(opaqueness);

    painter.setBrush(filledColour);

    painter.drawRect(0,0,stickyWidth,stickyHeight);

//    painter.drawConvexPolygon(stickyBrushPoints);

    painter.end();
}



// Note that we assume a square window here.
void penCursor::rebuildImage(void)
{
    switch( penMode )
    {
    case MODE_PRESENTATION:
        rebuildPresentationCursor();
        break;

    case MODE_OVERLAY:
        switch( type )
        {
        case CURS_NORMAL:  rebuildStandardCursor(); break;
        case CURS_BRUSH:   rebuildBrushCursor();    break;
        case CURS_STICKY:  rebuildStickyCursor();   break;
        case CURS_DELETE:  rebuildDeleteCursor();   break;
        case CURS_MOVE:    rebuildMoveCursor();     break;
        case CURS_TEXT:    rebuildTextCursor();     break;
        }
        break;

    default:
        qDebug() << "rebuildImage called when not in presentation or over4lay modes. Mode ="
                 << penMode;
    }

    update();   // Just in case it is visible
}


void penCursor::setOverlayMode(void)
{
    if( penMode == MODE_OVERLAY ) return;

    penMode = MODE_OVERLAY;

    rebuildImage(); // Re-create the cursor (it may have been the laser)

    show();
    raise();
}


void penCursor::setPresentationMode(void)
{
    if( penMode == MODE_PRESENTATION ) return;

    qDebug() << "Set penCursor to presentation mode.";

    penMode = MODE_PRESENTATION;

    rebuildImage(); // Cursor is now the pointer image (whatever that ends up as)

    if( presentationLaserIsOn )
    {
        show();
        raise();
    }
    else
    {
        hide();
    }
}


void penCursor::setInactiveMode(void)
{
    penMode = MODE_INACTIVE;

    hide();
}

void penCursor::togglePresentationPointer(void)
{
    if( presentationLaserIsOn )
    {
        qDebug() << "Toggled presentation pointer OFF (and hide it)";
        presentationLaserIsOn = false;
    }
    else
    {
        qDebug() << "Toggled presentation pointer ON";
        presentationLaserIsOn = true;
    }

    if( presentationLaserIsOn )
    {
        show();
        raise();
    }
    else
    {
        hide();
    }
}

