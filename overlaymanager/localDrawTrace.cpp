#include "localDrawTrace.h"

#include <QPainter>
#include <QDebug>

localDrawTrace::localDrawTrace(QImage *image, QWidget *parent) : QWidget(parent)
{

    //Public Annotation initialisation
    annotation = image;

    width  = image->width();
    height = image->width();

    annotation->fill(Qt::transparent);

    counter     = 0;
    drawing     = false;
    currentLine = 0;
    lastDraw    = 0;

    colour    = Qt::green;
    thickness = 2;
    localDrawFile.setFileName("/storage/emulated/0/Pictures/Cosneta/localDraw.pdf");

    //Private Notes initialisation
    pAnnotation = image;

    pAnnotation->fill(Qt::transparent);

    pCounter        = 0;
    pDrawing        = false;
    pLinesInArray   = false;
    pCurrentLine    = 0;
    pLastDraw       = 0;

    pColour         = Qt::green;
    pThickness      = 2;
    privateNoteFile.setFileName("/storage/emulated/0/Pictures/Cosneta/Private_Notes.pdf");
}

localDrawTrace::~localDrawTrace()
{

}

/*****************************************************
 *
 * Below section deals with Public Annotation
 *
 ****************************************************/

// Change the colour of the local draw to match the colour appearing on the host.
void localDrawTrace::setPenColour(QColor newColour)
{
    colour = newColour;
}

// Change the thickness of the local draw to match the thickness appearing on the host.
void localDrawTrace::setPenThickness(int newThickness)
{
    thickness = newThickness;
}

// Checks for any lines currently in the cache.
bool localDrawTrace::localDrawCachePresent(void)
{
    return lines.size() > 0;
}

/************************************************************************
 * Tracks user input to:
 * *draw between the previous point to the current one
 * *end the current line
 * *starts a new line
 * *clears any lines that expire
 ***********************************************************************/
void localDrawTrace::updateLocalDrawCache(int x, int y, bool draw)
{
    bool changed = false;

    // We use this instead of time
    counter ++;

    // Build up lines

    if( drawing )
    {
        if( draw )
        {
            QPoint   newPoint(x,y);
            QPainter paint(annotation);
            QPen     pen;
            pen.setCapStyle(Qt::RoundCap);
            pen.setWidth(thickness);
            pen.setColor(colour);

            paint.setPen(pen);
            paint.setRenderHint(QPainter::HighQualityAntialiasing);
            paint.setCompositionMode(QPainter::CompositionMode_Source);

            if( lines[currentLine].points.length() > 0 )
            {
                paint.drawLine(lines[currentLine].points.last(),newPoint);

                changed = true;
            }

            lines[currentLine].points.append(QPoint(x,y));
            localDrawSave();
        }
        else // ( ! draw )
        {
            // Record the end timepoint of the line
            lines[currentLine].endCounter = counter;
            lastDraw = counter;
            localDrawFile.close();
        }
    }
    else
    {
        if( draw )
        {
            // Start new line
            lineSegment_t newSegment;
            newSegment.colour    = colour;
            newSegment.thickness = thickness;
            newSegment.points.append(QPoint(x,y));
            lines.append(newSegment);

            currentLine = lines.size() - 1;
            linesInArray = true;
            localDrawSave();
        }
        else
        {
            // Still not drawing
            if ((counter - lastDraw > EXPIREY_UPDATES) && linesInArray)
            {
                qDebug() << "Is this called all the time?";
                resetDrawCache();
            }
        }
    }

        drawing = draw;

    // Check for line expirey

    bool redrawAll = false;

    for(int lineNo=lines.size()-1; lineNo>=0; lineNo --)
    {
        if( ((counter - lines[lineNo].endCounter) > EXPIREY_UPDATES) && !drawing )
        {
            lines.remove(lineNo);
            redrawAll = true;
        }
    }

    if( redrawAll )
    {
        annotation->fill(Qt::transparent);

        QPainter paint(annotation);
        QPen     pen;

        pen.setCapStyle(Qt::RoundCap);
        paint.setRenderHint(QPainter::HighQualityAntialiasing);
        paint.setCompositionMode(QPainter::CompositionMode_Source);

        for(int ln=0; ln<lines.size(); ln ++ )
        {
            pen.setWidth(lines[ln].thickness);
            pen.setColor(lines[ln].colour);

            paint.setPen(pen);

            for(int pt=1; pt<lines[ln].points.size(); pt++)
            {
                paint.drawLine(lines[ln].points[pt-1],lines[ln].points[pt]);
            }
        }

        changed = true;
    }
    if (changed)
    {
        redrawPrivateDraw();
    }

    if( changed ) update(); // ??? Is this necessary ???
}

void localDrawTrace::resetDrawCache(void)
{
    annotation->fill(Qt::transparent);

    lines.clear();
    linesInArray = false;
    redrawPrivateDraw();
}

/*****************************************************
 *
 * Below section deals with Private Notes
 *
 ****************************************************/

void localDrawTrace::setPPenColour(QColor newPColour)
{
    pColour = newPColour;
}

void localDrawTrace::setPPenThickness(int newPThickness)
{
    pThickness = newPThickness;
}

bool localDrawTrace::privateNoteDrawCachePresent(void)
{
    return pLines.size() > 0;
}

void localDrawTrace::updatePrivateLocalDrawCache(int x, int y, bool draw)
{
    bool changed = false;

    //We use this instead of time
    pCounter ++;

    //Build up lines
    if( pDrawing )
    {
        if( draw )
        {
            QPoint      newPPoint(x,y);
            QPainter    paint(pAnnotation);
            QPen        pPen;
            pPen.setCapStyle(Qt::RoundCap);
            pPen.setWidth(pThickness);
            pPen.setColor(pColour);

            paint.setPen(pPen);
            paint.setRenderHint(QPainter::HighQualityAntialiasing);
            paint.setCompositionMode(QPainter::CompositionMode_Source);

            if( pLines[pCurrentLine].pPoints.length() > 0 )
            {
                paint.drawLine(pLines[pCurrentLine].pPoints.last(), newPPoint);
                changed = true;
            }

            pLines[pCurrentLine].pPoints.append(QPoint(x,y));
            //privateNoteSave();
        }
        else
        {
            pLines[pCurrentLine].pEndCounter = pCounter;
            pLastDraw = pCounter;
            //privateNoteFile.close();
        }
    }
    else
    {
        if( draw )
        {
            //Start new line
            pLineSegment_t newSegment;
            newSegment.pColour      = pColour;
            newSegment.pThickness   = pThickness;
            newSegment.pPoints.append(QPoint(x,y));
            pLines.append(newSegment);

            pCurrentLine = pLines.size() - 1;
            pLinesInArray = true;
            //privateNoteSave();
        }
    }
    pDrawing = draw;
}

void localDrawTrace::redrawPrivateDraw(void)
{
    if(pLinesInArray)
    {
        pAnnotation->fill(Qt::transparent);
        QPainter paint(pAnnotation);
        QPen     pPen;
        pPen.setCapStyle(Qt::RoundCap);
        paint.setRenderHint(QPainter::HighQualityAntialiasing);
        paint.setCompositionMode(QPainter::CompositionMode_Source);
        for( int pln = 0; pln < pLines.size(); pln ++ )
        {
            pPen.setWidth(pLines[pln].pThickness);
            pPen.setColor(pLines[pln].pColour);

            paint.setPen(pPen);

            for( int ppt = 1; ppt < pLines[pln].pPoints.size(); ppt++ )
            {
                paint.drawLine(pLines[pln].pPoints[ppt-1], pLines[pln].pPoints[ppt] );
            }
        }
    }
}

void localDrawTrace::resetPrivateDrawCache(void)
{
    annotation->fill(Qt::transparent);

    pLines.clear();
    pLinesInArray = false;
}

void localDrawTrace::privateNoteUndo(void)
{
    if(pCurrentLine >= 0)
    {
        int lineNo = pCurrentLine;
        pLines.remove(lineNo);
        redrawPrivateDraw();
        pCurrentLine--;
        qDebug() << "Making sure this is called.";
    }
}

void localDrawTrace::fillPrivateBackground(QColor newColour)
{
    annotation->fill(newColour);
    redrawPrivateDraw();
}

void localDrawTrace::localDrawSave(void)
{
      QTextDocument doc;
      doc.setHtml( "<p>A QTextDocument can be used to present formatted text "
                   "in a nice way.</p>"
                   "<p align=center>It can be <b>formatted</b> "
                   "<font size=+2>in</font> <i>different</i> ways.</p>"
                   "<p>The text can be really long and contain many "
                   "paragraphs. It is properly wrapped and such...</p>" );
#ifndef Q_OS_IOS
       //QPrinter printer;
       //printer.setOutputFileName("/storage/emulated/0/Pictures/Cosneta/localDraw.pdf");
       //printer.setOutputFormat(QPrinter::PdfFormat);
       //doc.print(&printer);
       //printer.newPage();
#else
      qDebug() << "Local printing not currently supported...";
#endif
}

void localDrawTrace::privateNoteSave(void)
{
    QTextDocument doc;
    doc.setHtml( "<p>A QTextDocument can be used to present formatted text "
                 "in a nice way.</p>"
                 "<p align=center>It can be <b>formatted</b> "
                 "<font size=+2>in</font> <i>different</i> ways.</p>"
                 "<p>The text can be really long and contain many"
                 "paragraphs. It is properly wrapped and suuch...</p>" );
#ifndef Q_OS_IOS
    //QPrinter printer;
    //printer.setOutputFileName("/storage/emulated/0/Pictures/Cosneta/Private_Notes.pdf");
    //printer.setOutputFormat(QPrinter::PdfFormat);
    //doc.print(&printer);
    //printer.newPage();
#else
    qDebug() << "Local printing not currently supported...";
#endif
}
