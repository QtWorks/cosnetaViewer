#include <QPainter>
#include <QDebug>

#include "stickyNote.h"

#define MINIMUM_STICKY_WIDTH  50
#define MINIMUM_STICKY_HEIGHT 50

#define USE_VO false // Set to true to mean we use the Vision Objects MyScript Web Service so resize stickNote for words

#define DRAGBAR_HEIGHT        15


stickyNote::stickyNote(int height, int width, int penNum,
                       bool *penInTextModeRef, bool *brushActionRef,
                       QColor defaultPenColour[], int startThickness[],
                       QColor colour, QWidget *parent) :
                                          QWidget(parent, Qt::FramelessWindowHint        |
                                                          Qt::X11BypassWindowManagerHint |
                                                          Qt::WindowStaysOnTopHint)
{
    resize(height, width);

    backgroundColour      = colour;
    penInTextMode         = penInTextModeRef;
    brushAction           = brushActionRef;
    // This could be used to block others from a private note - future.
    ownerPen = penNum;

    annotationsImage      = new QImage(width, height, QImage::Format_ARGB32_Premultiplied);
    highlights            = new QImage(width, height, QImage::Format_ARGB32_Premultiplied);
    textLayerImage        = new QImage(width, height, QImage::Format_ARGB32_Premultiplied);

    annotationsImage->fill(Qt::transparent);
    highlights->fill(Qt::transparent);
    textLayerImage->fill(Qt::transparent);

    annotations = new overlay(annotationsImage,highlights,textLayerImage,
                              defaultPenColour, colour, false, NULL, this);

    for(int p=0; p<MAX_SCREEN_PENS; p++)
    {
        annotations->setPenThickness(p,startThickness[p]);
    }

    lastPenButtons.leftMouse       = false;
    lastPenButtons.centreMouse     = false;
    lastPenButtons.rightMouse      = false;
    lastPenButtons.scrollPlus      = false;
    lastPenButtons.scrollMinus     = false;
    lastPenButtons.modeToggle      = false;
    lastPenButtons.precisionToggle = false;
    lastPenButtons.gestureToggle   = false;

    for(int p=0; p<MAX_SCREEN_PENS; p++)
    {
        penIsDrawing[p]             = false;
        waitForPenButtonsRelease[p] = true;
    }

    // Pass mouse events through to main window so mouse as pen works over stickies
    setAttribute(Qt::WA_TransparentForMouseEvents,true);
}

void stickyNote::applyPenData(int penNum, QPoint inPenPos, buttonsActions_t &buttons)
{
    // Draw actions
    if( penInTextMode[penNum] )
    {
        if( buttons.leftMouseAction == CLICKED )
        {
            qDebug() << "stickyNote: applyPenData: setTextPosFromPixelPos" << inPenPos;

            annotations->setTextPosFromPixelPos(penNum, inPenPos.x(), inPenPos.y());

            return;
        }
    }

    if( brushAction[penNum] )
    {
        // If left click, stamp an image; if right click, exit brush mode

        if( buttons.rightMouseAction == CLICKED )
        {
            qDebug() << "Deselected brush mode with right mouse.";

            brushAction[penNum] = false;
        }
        else if( buttons.leftMouseAction == CLICKED )
        {
            qDebug() << "Brush paint.";

            annotations->CurrentBrushAction(penNum,inPenPos.x(),inPenPos.y());

            waitForPenButtonsRelease[penNum] = true;
            return;
        }

        // Allow for enlarge/diminish with the MODE+SCROLL_UP or MODE+SCROLL_DOWN

        if( buttons.centreMouse )
        {
            if( buttons.scrollMinusAction == CLICKED )
            {
                annotations->zoomBrush(penNum, -10);
            }
            else if( buttons.scrollPlusAction == CLICKED )
            {
                annotations->zoomBrush(penNum, +10);
            }
        }

        penIsDrawing[penNum] = false;

        return;
    }
    else if( ! penIsDrawing[penNum] && buttons.leftMouse )
    {
        qDebug() << "Sticky start pen draw.";

        annotations->startPenDraw(penNum);
    }
    else if( penIsDrawing[penNum] && ! buttons.leftMouse )
    {
        qDebug() << "Sticky stop pen draw.";

        annotations->stopPenDraw(penNum);
    }

    penIsDrawing[penNum] = buttons.leftMouse;
}


void stickyNote::penPositions(bool *present, int *xLoc, int *yLoc)
{
    annotations->penPositions(present, xLoc, yLoc);
}


void stickyNote::setTextPosFromPixelPos(int penNum, int x, int y)
{
    qDebug() << "stickyNote: setTextPosFromPixelPos" << x << y;

    annotations->setTextPosFromPixelPos(penNum, x,y);
}


void stickyNote::addTextCharacterToOverlay(int penNum, quint32 charData)
{
//    qDebug() << "stickyNote: addTextCharacterToOverlay" << penNum << charData;

    if( penInTextMode[penNum] )
    {
        annotations->addTextCharacterToOverlay(penNum, APP_CTRL_MODKEY_FROM_RAW_KEYPRESS(charData));
    }
}

void stickyNote::setNoteColour(int penNum, QColor newColour)
{
    qDebug() << "stickyNote: setNoteColour" << penNum << newColour;

    annotations->setBackground(newColour);
    backgroundColour = newColour;

    update();
}

void stickyNote::penTypeDraw(int penNum)
{
    annotations->penTypeDraw(penNum);
}

void stickyNote::penTypeText(int penNum)
{
    annotations->penTypeText(penNum);
}

void stickyNote::penTypeEraser(int penNum)
{
    annotations->penTypeEraser(penNum);
}

void stickyNote::penTypeHighlighter(int penNum)
{
    annotations->penTypeHighlighter(penNum);
}

void stickyNote::setPenColour(int penNum, QColor newColour)
{
    qDebug() << "stickyNote::setPenColour" << penNum << newColour;

    annotations->setPenColour(penNum, newColour);
}


// NB The caller wont know the new size and therefore must check for any
// resulting overlap after the change, and ondo it by calling us here to
// fix it. This is all a bit nasty, of course.
bool stickyNote::scaleStickySize(double scaleFactor)
{
    if( scaleFactor<=0.0 )
    {
        qDebug() << "Sticky scale: bad factor:" << scaleFactor;
        return false;
    }

    int newWidth  = width()  * scaleFactor;
    int newHeight = height() * scaleFactor;

    if( newWidth == width() )
    {
        if( scaleFactor>1.0 ) newWidth ++;
        else                  newWidth --;
    }
    if( newHeight == height() )
    {
        if( scaleFactor>1.0 ) newHeight ++;
        else                  newHeight --;
    }

    QWidget *prnt = (QWidget *)this->parent();
    if( newWidth <MINIMUM_STICKY_WIDTH || newHeight<MINIMUM_STICKY_HEIGHT ) return false;
    if( newWidth > prnt->width()       || newHeight>prnt->width() )         return false;

    resize(newWidth,newHeight);

    annotationsImage->scaled(newWidth,newHeight);
    highlights->scaled(newWidth,newHeight);
    textLayerImage->scaled(newWidth,newHeight);

    update();

    return true;
}


void stickyNote::paintEvent(QPaintEvent *ev)
{
    QPainter painter(this);

    painter.fillRect(QRect(0,0,width(),height()), backgroundColour);

    // Add the overlay, highlights and text
    painter.drawImage(0,0,*annotationsImage);
    painter.drawImage(0,0,*highlights);
    painter.drawImage(0,0,*textLayerImage);

    // Draw a drag bar at the top
    QPen pen;
    pen.setColor(Qt::darkYellow);
    pen.setWidth(2);
    painter.setPen(pen);

    for(int l=2; l<DRAGBAR_HEIGHT; l+=4)
    {
        painter.drawLine(2,l,width()-2,l);
    }

    painter.end();
}


QRect stickyNote::screenRect(void)
{
    return QRect(pos().x(), pos().y(), width(), height() );
}


void stickyNote::setStickyDragStart(QPoint startPos)
{
    dragStartPoint = startPos;
}


void stickyNote::dragToo(QPoint newPos)
{
    QPoint newCoords = pos() + (newPos - dragStartPoint);

    dragStartPoint   = newPos;  // Required as we are doing this relative to pos() which we changed last time.

    move(newCoords);
}


bool stickyNote::clickIsInDragBar(QPoint checkPos)
{
    return checkPos.y()>=0 && checkPos.y()<DRAGBAR_HEIGHT;
}


// Take out old text recognition demonstration. Not deleted so it can be added to
// overlay.cpp (or more likely, overlaywindow.cpp).
#if 0
stickyNote::stickyNote(int height, int width, int penNum, QColor colour, QWidget *parent) :
                                          QWidget(parent, Qt::FramelessWindowHint        |
                                                          Qt::X11BypassWindowManagerHint |
                                                          Qt::WindowStaysOnTopHint)
{
    if (USE_VO)
    {
        // JF added to make sticky rectangular to better facilitate writing words
        // TODO fix so as retains shape on resize
        height = height * 2; // Height is horizontal - deliberate?
    }

    resize(height, width);

    drawImage = QImage(height,width,QImage::Format_ARGB32_Premultiplied);
    drawImage.fill(colour);
    backgroundColour = colour;

    text = new textOverlay(height, width);

    if (USE_VO) {
        // TODO - create shapeOverlay class
        //shapeOverlay = new shapeOverlay(height, width);
    }

    lastPenButtons.leftMouse       = false;
    lastPenButtons.centreMouse     = false;
    lastPenButtons.rightMouse      = false;
    lastPenButtons.scrollPlus      = false;
    lastPenButtons.scrollMinus     = false;
    lastPenButtons.modeToggle      = false;
    lastPenButtons.precisionToggle = false;
    lastPenButtons.gestureToggle   = false;

    // Development only
    textOverlayDebug = QImage(50,100,QImage::Format_ARGB32_Premultiplied);
    textOverlayDebug.fill(Qt::transparent);

    stroke.clear();

    nextStrokeID     = 0;
    lastHWRRequestTm = 0;
    tmrDeciSecs      = 0;

    connect(&tmr, SIGNAL(timeout()), this, SLOT(timerTick()));
    if (USE_VO)
    {
        tmr.start(500);   // 500ms, ie 2Hz // JF increase time from 100ms/10Hz to cope with words
    } else
    {
        tmr.start(100);   // 100ms, ie 10Hz
    }

    ownerPen = penNum;

    // Pass mouse events through so mouse as pen works over stickies
    setAttribute(Qt::WA_TransparentForMouseEvents,true);
}



void stickyNote::applyPenData(int penNum, QPoint inPenPos, buttonsActions_t &buttons)
{
    // NB Currently we only allow the sticky's creator to write on it.
    if( ownerPen != penNum ) return;

    // Move to local coordinates
    QPoint penPos = inPenPos - pos();

    if( GENERIC_ZOOM_IN(buttons) )
    {
        lastPenButtons = buttons;
        lastPos        = penPos;

        scaleStickySize(1.1);

        return;
    }

    if( GENERIC_ZOOM_OUT(buttons) )
    {
        lastPenButtons = buttons;
        lastPos        = penPos;

        scaleStickySize(0.9);

        return;
    }

    // Left mouse actions (draw lines)
    if( buttons.leftMouse == true )
    {
        if( lastPenButtons.leftMouse != true )
        {
            // New stroke
            stroke_t *newStroke = new stroke_t;
            newStroke->point.append(penPos);
            newStroke->complete = false;

            stroke.append(newStroke);

            qDebug() << "start new stroke";
        }
        else
        {
            // Continue stroke - but no repeated points please
            if( penPos != lastPos && penPos.y() > DRAGBAR_HEIGHT )
            {
                stroke.last()->point.append(penPos);
                stroke.last()->addTime = tmrDeciSecs;  // Allows us to time-out "off sticky" lines

                QPainter painter(&drawImage);
                QPen     pen;

                pen.setColor(Qt::black);
                pen.setWidth(2);

                painter.setPen(pen);
                painter.setRenderHint(QPainter::Antialiasing);

                painter.drawLine(lastPos,penPos);

                painter.end();

                repaint();      // Ensure that this gets to the screen)
            }
        }
    }
    else // buttons.leftMouse == false
    {
        if( lastPenButtons.leftMouse == true )
        {
            // End stroke (and record time)
            stroke.last()->complete = true;
            stroke.last()->addTime  = tmrDeciSecs;
            stroke.last()->strokeID = nextStrokeID;

            nextStrokeID ++;

            qDebug() << "End stroke. Num strokes =" << stroke.size();
        }
    }

    lastPenButtons = buttons;
    lastPos        = penPos;
}



void stickyNote::paintEvent(QPaintEvent *ev)
{
    QPainter painter(this);

    // Add the overlay
    painter.drawImage(0,0,drawImage);
    painter.drawImage(0,20,textOverlayDebug);
    text->paintImage(&painter);

    // Draw a drag bar at the top
    QPen pen;
    pen.setColor(Qt::darkYellow);
    pen.setWidth(2);
    painter.setPen(pen);

    for(int l=2; l<DRAGBAR_HEIGHT; l+=4)
    {
        painter.drawLine(2,l,width()-2,l);
    }

    painter.end();
}



void stickyNote::timerTick(void)
{
#ifndef Q_OS_ANDROID
    tmrDeciSecs ++;

    // Check for responses from the handwriting thread

    // Trim out old strokes
    bool changed = false;

    // Delete from the end (most recent) as we are using indexed addressing.
    for( int s=(stroke.size()-1); s>=0; s-- )
    {
        // Timeout strokes after 2 seconds (20 deciSecs)
        if( stroke[s]->complete        &&
            (tmrDeciSecs - stroke[s]->addTime)>TEXT_STROKE_DELETE_TM )
        {
            delete stroke[s];
            stroke.remove(s);

            changed = true;
        }
    }

    // Do we have any responses to previous requests?
    if( ! responses.isEmpty() )
    {
        qDebug() << "Received a response from handwritingRecognition";
        // QFont f; want "fixed pitch", and a limited set of sizes

        hwrResult_t result = responses.dequeue();

        if (USE_VO) {
            // See if any shapes have been returned
            if (result.shapeList.size() > 0 )
            {
                qDebug() << "Found " << result.shapeList.size() << " shape(s)";
            }
            else
            {
                qDebug() << "No shapes found in result.shapeList";
            }
        }

        // Need to account for text & shapes in results struct
        if( result.match.size() > 0 )
        {

            if (USE_VO) {
                // When "writing", start at minX/2 and add 20 for each character
                int x_int = result.minX;
                int y_int = result.minY;

                // Loop through each result.match value
                for (int i = 0; i < result.match.size(); ++i)
                {
                    qDebug() << "Draw char" << result.match[i].ch << "at" << x_int << y_int;
                    // Fix height at 20 for now, increment x by 20
                    text->addCharacter(result.match[i].ch, x_int, y_int, 20, Qt::black);
                    x_int = x_int + 20;
                }
            }

            // Draw any characters
            qDebug() << "Draw char" << result.match[0].ch << "at" << result.minX << result.minY
                     << "Ht" << (result.maxY-result.minY);

            text->addCharacter(result.match[0].ch, result.minX,result.minY,
                               result.maxY-result.minY, Qt::black);
#if 0
            // Debug
            QString opts;
            for(int s=0; s<result.match.size(); s++)
            {
                opts += result.match[s].ch;
            }
            for(int l=5; l>0; l--)
            {
                matches[l] = matches[l-1];
            }
            matches[0] = opts;

            textOverlayDebug.fill(Qt::transparent);
            QPainter p(&textOverlayDebug);
            p.setBrush(Qt::green);
            for(int line=0; line<6; line++)
            {
                p.drawText(0, 10+line*textOverlayDebug.height()/6, matches[line]);
            }
#endif
            repaint();

            // Delete any consumed strokes that haven't timed out
            for( int s=(stroke.size()-1); s>=0; s-- )
            {
                // Timeout strokes after 2 seconds (20 deciSecs)
                if( stroke[s]->complete && stroke[s]->addTime < result.lastStrokeTm )
                {
                    delete stroke[s];
                    stroke.remove(s);

                    changed = true;
                }
            }
        }
    }

    // Go through the remaining strokes (if done) and if enough of a delay, check for strokes
    if( stroke.size() > 0 )
    {
        int lastAddTm = 0;

        for( int s=0; s<stroke.size(); s++ )
        {
            if( stroke[s]->complete ) lastAddTm = stroke[s]->addTime;
        }

        if( lastAddTm != lastHWRRequestTm && (tmrDeciSecs-lastAddTm)>MIN_TM_TO_LETTER_CHECK )
        {
            // New stroke, so could try again
            hwrRequest_t request;

            request.answerQueue  = &(responses);
            request.lastStrokeTm = lastAddTm;
            for(int st=0; st<stroke.size(); st++)
            {
                if( stroke[st]->complete )
                {
                    request.strokePoints.push_back(stroke[st]->point);
                }
            }
            requestQueue->enqueue(request);

            // Don't try again until we have a new stroke
            lastHWRRequestTm = lastAddTm;

            qDebug() << "Sent handwriting recognition request with" << request.strokePoints.size() << "strokes.";
        }
    }

    // If we have something to interpret, send it to the handwriting thread

    // Redraw note if the strokes have changed
    if( changed )
    {
        drawImage.fill(backgroundColour);

        QPainter paint(&drawImage);
        QPen     pen;

        pen.setColor(Qt::black);
        pen.setWidth(2);

        paint.setPen(pen);
        paint.setRenderHint(QPainter::Antialiasing);

        for( int s=0; s<stroke.size(); s++ )
        {
            // Draw the stroke
            for(int pt=1; pt<stroke[s]->point.size(); pt++)
            {
                paint.drawLine(stroke[s]->point[pt-1], stroke[s]->point[pt]);
            }
        }

        paint.end();

        repaint();      // Ensure that this gets to the screen)
    }
#endif
}

#endif
