#include "replayManager.h"
#include "overlay.h"

#include <QApplication>
#include <QDesktopWidget>
#include <QPainter>
#include <QFile>
#include <QBuffer>
#include <iostream>

#include <QDebug>





replayManager::replayManager(QImage  *drawImage,
                             QImage  *highlightImage,
                             QImage  *txtImage,
                             QColor  *initColour,
                             QWidget *parent) : QWidget(parent)
{
    // NB We pass ihn our parent as we are not a graphical entity
    replayAnnotations = new overlay(drawImage,highlightImage,txtImage,initColour,Qt::transparent,false,NULL,parent);

    w = drawImage->width();
    h = drawImage->height();

    recording        = false;
    playingBack      = false;

    behindTheAnnotations = NULL;

    for(int pen=0; pen<MAX_SCREEN_PENS; pen++)
    {
        penPresent[pen] = false;

        penColour[pen]    = initColour[pen];
        penThickness[pen] = 2;
        penDrawstyle[pen] = 1;

        replayPenCursor[pen].hide();
        replayPenCursor[pen].updatePenColour( initColour[pen] );
    }

    behindTheAnnotations  = NULL;
}




void replayManager::writeTimePointToFile(void)
{
    int           tm = sessionTime.elapsed() / 10;             // 1/100 sec, not 1/1000
    unsigned char timeWord[4] = { ACTION_TIMEPOINT,
                                  (uchar)(( tm >> 16) & 255),
                                  (uchar)(( tm >>  8) & 255),
                                  (uchar)(tm & 255) };
    unsigned char top = (tm >> 24) & 255;

    // Only save if the timepoint has moved on
    if( lastSaveTimepoint[0] != top          || lastSaveTimepoint[1] != timeWord[1] ||
        lastSaveTimepoint[2] != timeWord[2]  || lastSaveTimepoint[3] != timeWord[3]   )
    {
        if( saveStream.writeRawData((const char *)timeWord,sizeof(timeWord)) < 0 )
        {
            std::cout << saveFile.errorString().toStdString() << std::endl;
        }

        lastSaveTimepoint[0] = top;
        lastSaveTimepoint[1] = timeWord[1];
        lastSaveTimepoint[2] = timeWord[2];
        lastSaveTimepoint[3] = timeWord[3];
    }
}



void    replayManager::setBackground(QColor bgColour)
{
    if( playingBack || ! recording ) return;

    qint32 colHeader = ACTION_BACKGROUND_COLOUR << 24;
    qint32 colour    = bgColour.red()  << 24 | bgColour.green() << 16 |
                       bgColour.blue() <<  8 | bgColour.alpha();

    writeTimePointToFile();

    if( bgColour.alpha() < 255 )
    {
        QPixmap background = QPixmap::grabWindow( QApplication::desktop()->winId() );

        QByteArray bytes;
        QBuffer    buffer(&bytes);
        buffer.open(QIODevice::WriteOnly);
        background.save(&buffer, "PNG");

        // TODO: sanity check for size<16Mb.
        unsigned char imgHeader[4] = {  ACTION_BACKGROUND_PNG,
                                        (uchar)(( bytes.size() >> 16 ) & 255),
                                        (uchar)(( bytes.size() >>  8 ) & 255),
                                        (uchar)(( bytes.size()       ) & 255)  };

        saveStream.writeRawData((const char *)imgHeader,sizeof(imgHeader));
        qDebug() << "Wrote " <<
                    saveStream.writeRawData(bytes.constData(),bytes.size()) // ;
                 << " bytes of image data. Expected " << bytes.size();

        saveFile.flush();
    }
else qDebug() << "Background not transparent, so do not save.";

    saveStream << colHeader;
    saveStream << colour;
    qDebug() << "replayManager::setBackground: " << bgColour;
}



void replayManager::loadReplayBackgroundPNGImage(int length)
{
    if( ! playingBack ) return;

    QByteArray bytes;
    bytes.resize(length);

    replayFile.read(bytes.data(),length);

    QImage replayBackground;
    replayBackground.loadFromData((unsigned char *)bytes.data(),length,"PNG");

    QPainter paint(behindTheAnnotations);
    paint.setCompositionMode(QPainter::CompositionMode_Source);
    paint.drawImage(0,0,replayBackground);
    paint.end();
}



void replayManager::loadReplayBackgroundJPGImage(int length)
{
    if( ! playingBack ) return;

    QByteArray bytes;
    bytes.resize(length);

    replayFile.read(bytes.data(),length);

    QImage replayBackground;
    replayBackground.loadFromData((unsigned char *)bytes.data(),length,"JPG");

    QPainter paint(behindTheAnnotations);
    paint.setCompositionMode(QPainter::CompositionMode_Source);
    paint.drawImage(0,0,replayBackground);
    paint.end();
}



void    replayManager::setPenColour(int penNum, QColor col)
{
    if( playingBack || ! recording ) return;

    if( penNum>=0 && penNum<MAX_SCREEN_PENS ) penColour[penNum] = col;

    qint32 header = ACTION_PENCOLOUR << 24 | penNum;
    qint32 data   = col.red() << 24 | col.green() << 16 | col.blue() << 8 | col.alpha();

    writeTimePointToFile();

    saveStream << header;
    saveStream << data;
}

void    replayManager::setPenThickness(int penNum, int thick)
{
    if( playingBack || ! recording ) return;

    if( penNum>=0 && penNum<MAX_SCREEN_PENS ) penThickness[penNum] = thick;

    qint32 data = ACTION_PENWIDTH << 24 | thick << 8 | penNum;

    writeTimePointToFile();

    saveStream << data;
}



void    replayManager::penTypeDraw(int penNum)
{
    if( playingBack || ! recording ) return;

    if( penNum>=0 && penNum<MAX_SCREEN_PENS ) penDrawstyle[penNum] = TYPE_DRAW;

    qint32 data = ACTION_PENTYPE << 24 | TYPE_DRAW << 8 | penNum;

    writeTimePointToFile();

    saveStream << data;
}

void    replayManager::penTypeText(int penNum)
{
    if( playingBack || ! recording ) return;

    qint32 data = ACTION_PENTYPE << 24 | penNum;

    writeTimePointToFile();

    saveStream << data;
}

void    replayManager::penTypeHighlighter(int penNum)
{
    if( playingBack || ! recording ) return;

    if( penNum>=0 && penNum<MAX_SCREEN_PENS ) penDrawstyle[penNum] = TYPE_HIGHLIGHTER;

    qint32 data = ACTION_PENTYPE << 24 | TYPE_HIGHLIGHTER << 8 | penNum;

    writeTimePointToFile();

    saveStream << data;
}

void    replayManager::penTypeEraser(int penNum)
{
    if( playingBack || ! recording ) return;

    if( penNum>=0 && penNum<MAX_SCREEN_PENS )
    {
        penDrawstyle[penNum] = TYPE_ERASER;

        qint32 data = ACTION_PENTYPE << 24 | TYPE_ERASER << 8 | penNum;

        writeTimePointToFile();

        saveStream << data;
    }
}



void replayManager::startPenDraw(int penNum)
{
    if( playingBack || ! recording ) return;

    if( penNum>=0 && penNum<MAX_SCREEN_PENS )
    {
        qint32 data = ACTION_STARTDRAWING << 24 | penNum;

        writeTimePointToFile();

        saveStream << data;
    }
}


void replayManager::stopPenDraw(int penNum)
{
    if( playingBack || ! recording ) return;

    if( penNum>=0 && penNum<MAX_SCREEN_PENS )
    {
        qint32 data = ACTION_STOPDRAWING << 24 | penNum;

        writeTimePointToFile();

        saveStream << data;
    }
}





void    replayManager::nextPage(void)
{
    qDebug() << "replayManager::nextPage()";

    if( playingBack || ! recording ) return;

    qint32 data = ACTION_NEXTPAGE << 24;

    writeTimePointToFile();

    saveStream << data;
}

void    replayManager::previousPage(void)
{
    if( playingBack || ! recording ) return;

    qint32 data = ACTION_PREVPAGE << 24;

    writeTimePointToFile();

    saveStream << data;
}

void    replayManager::gotoPage(int number)
{
    if( playingBack || ! recording ) return;

    qint32 data = ACTION_GOTOPAGE << 24 | (number & 0xFFFFFF);

    writeTimePointToFile();

    saveStream << data;
}

void    replayManager::clearOverlay(void)
{
    if( playingBack || ! recording ) return;

    qint32 data = ACTION_CLEAR_SCREEN << 24;

    writeTimePointToFile();

    saveStream << data;
}


void    replayManager::setTextPosFromPixelPos(int penNum, int x, int y)
{
    if( playingBack || ! recording ) return;

    qint32 data      = ACTION_SET_TEXT_CURSOR_POS << 24 | (penNum & 255);
    qint32 penCoords = (x & 65535) << 16 | (y & 65535);;

    writeTimePointToFile();

    saveStream << data;
    saveStream << penCoords;
}

void    replayManager::addTextCharacterToOverlay(int penNum, quint32 charData)
{
    if( playingBack || ! recording ) return;

    qint32 data = ACTION_APPLY_TEXT_CHAR << 24 | (penNum & 255);

    writeTimePointToFile();

    saveStream << data;
    saveStream << charData;
}


void    replayManager::penPositions(bool newPenPresent[], int xLoc[], int yLoc[])
{
    if( playingBack || ! recording ) return;

    bool changed = false;
    int  penMask = 0;

    for(int i=0; i<MAX_SCREEN_PENS; i++)
    {
        if( penPresent[i] ) penMask |= 1<<i;

        if( ! changed )  // Save the test if have found a difference as usually true
        {
            if( newPenPresent[i] != penPresent[i]        ||
                xLoc[i] != penPosPrevX[i] || yLoc[i] != penPosPrevY[i]   )
            {
                changed = true;
            }
        }
        penPresent[i]   = newPenPresent[i];
        penPosPrevX[i]  = xLoc[i];
        penPosPrevY[i]  = yLoc[i];
    }

    if( changed )
    {
        writeTimePointToFile();

        qint32  penMaskWord = ACTION_PENPOSITIONS << 24 | penMask;

        saveStream << penMaskWord;

        qint32  penCoords;

        for(int i=0; i<MAX_SCREEN_PENS; i++)
        {
            if( penPresent[i] )
            {
                penCoords = (xLoc[i] & 65535) << 16 | (yLoc[i] & 65535);

                saveStream << penCoords;
            }
        }
    }
}


void    replayManager::undoPenAction(int penNum)
{
    if( playingBack || ! recording ) return;

    qint32 data = ACTION_UNDO << 24 | penNum;

    writeTimePointToFile();

    saveStream << data;
}

void    replayManager::redoPenAction(int penNum)
{
    if( playingBack || ! recording ) return;

    qint32 data = ACTION_REDO << 24 | penNum;

    writeTimePointToFile();

    saveStream << data;
}

void    replayManager::repeatPenAction(int penNum, int x, int y)
{
    if( playingBack || ! recording ) return;

    qint32 data = ACTION_DOBRUSH << 24 | penNum;

    writeTimePointToFile();

    saveStream << data;

    data   = ((65535 & x) << 16) | (y & 65535);

    saveStream << data;
}

void    replayManager::brushFromLast(int penNum)
{
    if( playingBack || ! recording ) return;

    qint32 data = ACTION_BRUSHFROMLAST << 24 | penNum;

    writeTimePointToFile();

    saveStream << data;

    return;
}

void    replayManager::explicitBrush(int penNum, shape_t shape, int param1, int param2)
{
    if( playingBack || ! recording ) return;

    qint32 data;

    switch( shape )
    {
    case SHAPE_TRIANGLE:
        data = ACTION_TRIANGLE_BRUSH << 24 | penNum;
        break;

    case SHAPE_BOX:
        data = ACTION_BOX_BRUSH << 24 | penNum;
        break;

    case SHAPE_CIRCLE:
        data = ACTION_CIRCLE_BRUSH << 24 | penNum;
        break;

    case SHAPE_NONE:
        qDebug() << "SHAPE_NONE in replayManager::explicitBrush()";
        return;

    default:
        qDebug() << "Ignoring unexpected shape:" << shape;
        return;
    }

    writeTimePointToFile();

    saveStream << data;

    data   = ((65535 & param1) << 16) | (param2 & 65535);

    saveStream << data;

    return;
}




void replayManager::zoomBrush(int penNum, int scalePercent)
{
    if( playingBack || ! recording ) return;

    qint32 data = ACTION_BRUSH_ZOOM << 24 | ((scalePercent&65535) << 8) | penNum;

    saveStream << data;

    return;
}




QImage *replayManager::imageData(void)
{
    return replayAnnotations->imageData();
}



bool    replayManager::saveDiscussion(QString fileName)
{
    qDebug() << "saveDiscussion()";

    if( playingBack || recording )
    {
        qDebug() << " abort as replaying (or already recording).";

        // TODO: Generate an error pop-up
        return false;
    }

    saveFile.setFileName(fileName);

    if( ! saveFile.open(QIODevice::WriteOnly|QIODevice::Truncate) )
    {
        // TODO: add a wrapper in main.cpp to auto pull down this (as cosneta Pens wont)
        //       or write a signal pipe in main...
        // Failed to start saving the discussion.
        return false;
    }

    recording = true;

    saveStream.setDevice(&saveFile);
    saveStream.setVersion(QDataStream::Qt_5_0);
    // TODO: write an application version number

    // Ensure any pens are new and trigger a write
    for(int p=0; p<MAX_SCREEN_PENS; p++)
    {
        penPresent[p] = false;
    }
    // When flag is set, will record a screen snap too
    lastSaveTimepoint[0] = 0xFF;  // Ensure that the 'previous timepoint' triggers a write
    sessionTime.restart();        // Keep a time in milli-seconds

    qint32 header;
    qint32 data;

    // Save current pen colours and thicknesses (including those of inactive pens)
    for(int penNum=0; penNum<MAX_SCREEN_PENS; penNum++)
    {
        header = ACTION_PENCOLOUR << 24 | penNum;
        data   = penColour[penNum].red() << 24 | penColour[penNum].green() << 16 |
                 penColour[penNum].blue() << 8 | penColour[penNum].alpha();
        saveStream << header;
        saveStream << data;
        std::cerr << "Save Pen " << penNum << std::hex << " colour = #" << ((int)(data))
                  << std::dec << "(" << penColour[penNum].name().toStdString() << ")";
        data = ACTION_PENWIDTH << 24 | penThickness[penNum] << 8 | penNum;
        saveStream << data;

        data = ACTION_PENTYPE  << 24 | penDrawstyle[penNum] << 8 | penNum;
        saveStream << data;
    }

    // Record the current screen size before anything that might be drawn
    header = ACTION_SCREENSIZE << 24;
    data   = ((65535 & w) << 16) | (h & 65535);

    saveStream << header;
    saveStream << data;

    // TODO: may want to start a save session with the current background
    setBackground(Qt::transparent);

    return true;
}

void    replayManager::finishUpSaving(void)
{
    if( ! recording ) return;

    saveFile.close();

    recording = false;
}

bool    replayManager::currentlyRecording(void)
{
    return recording;
}
bool    replayManager::currentlyPlaying(void)
{
    return playingBack;
}



bool    replayManager::replayDiscussion(QString fileName, QImage *background)
{
    if( playingBack ) return false;
    if( recording )   finishUpSaving();

    replayFile.setFileName(fileName);

    if( ! replayFile.open(QIODevice::ReadOnly) )
    {
        // Failed to start playing the saved discussion.
        return false;
    }

    // Copy the current replay pens from the base values
    replayAnnotations->setDefaults();

    behindTheAnnotations = background;

    replayAnnotations->imageData()->fill(Qt::transparent);
    behindTheAnnotations->fill(Qt::transparent);

    playingBack = true;

    for(int i=0; i<sizeof(playbackTimepoint); i++ ) playbackTimepoint[i] = 0;

    return true;
}


int     replayManager::nextPlaybackStep(void)             // Returns msec till next change
{
    bool  penPresent[MAX_SCREEN_PENS];                 // Moved here due to the compiler's moans.
    int   xLoc[MAX_SCREEN_PENS], yLoc[MAX_SCREEN_PENS];

    if( ! playingBack ) return -1;

    unsigned char        tagBytes[(1+MAX_SCREEN_PENS)*sizeof(quint32)];  // Yeah, *4 is sweeter, but less informative
    unsigned char       *ptr;
    qint32               timeDiff;
    int                  penNum;
    bool                 timeFound   = false;
    bool                 error       = false;
    QColor               col;
    QColor               backgroundColour;
    QPainter             paint;

    while( ! timeFound && ! error )
    {
        if( replayFile.read((char *)tagBytes,4) <= 0 )
        {
            error = true;  // END of playback (0 => EOF)
        }
        else
        {
            qDebug() << ": " << ACTION_DEBUG(tagBytes[0]) << "(" << ((int)(tagBytes[0])) << ")";
            switch( tagBytes[0] )
            {
            case ACTION_TIMEPOINT:
                timeDiff = ((tagBytes[1] - playbackTimepoint[1])<<16) +
                           ((tagBytes[2] - playbackTimepoint[2])<<8)  +
                            (tagBytes[3] - playbackTimepoint[3]);
                playbackTimepoint[1] = tagBytes[1];
                playbackTimepoint[2] = tagBytes[2];
                playbackTimepoint[3] = tagBytes[3];
                timeFound = true;
                break;

            case ACTION_SCREENSIZE:
                if( replayFile.read((char *)(4+tagBytes),4) < 0 ) {error =true; break;}
                // Allegedly, this isn't a memory leak as Qt does stuff in '='.
                w = (tagBytes[4]<<8)|tagBytes[5];
                h = (tagBytes[6]<<8)|tagBytes[7];

                replayAnnotations->setScreenSize(w,h);
                behindTheAnnotations->scaled(QSize(w,h));

                break;

            case ACTION_BACKGROUND_PNG:
                loadReplayBackgroundPNGImage((tagBytes[1]<<16) + (tagBytes[2]<<8) + (tagBytes[3])); // length = bottom 24 bits
                break;

            case ACTION_BACKGROUND_JPG:
                loadReplayBackgroundJPGImage((tagBytes[1]<<16) + (tagBytes[2]<<8) + (tagBytes[3])); // length = bottom 24 bits
                break;

            case ACTION_BACKGROUND_COLOUR:
                if( replayFile.read((char *)(4+tagBytes),4) < 0 )
                {
                    error = true;
                    break;
                }
                backgroundColour = QColor(tagBytes[4],tagBytes[5],tagBytes[6],tagBytes[7]);
                qDebug("Background colour #%x %x %x : %x",tagBytes[4],tagBytes[5],tagBytes[6],tagBytes[7]);
                replayAnnotations->setBackground(backgroundColour);
                if( backgroundColour.alpha() < 255 )
                {
                    // NB This wont work for multiple transparent backgrounds (not that we have these yet)
                    paint.begin(behindTheAnnotations);
                    paint.setCompositionMode(QPainter::CompositionMode_Overlay);
                    paint.setPen(QPen(backgroundColour));
                    paint.drawRect(behindTheAnnotations->rect());
                    paint.end();
                }
                else
                {
                    behindTheAnnotations->fill(backgroundColour);
                }

                replayAnnotations->setBackground(backgroundColour);
                break;

            case ACTION_CLEAR_SCREEN:
                replayAnnotations->clearOverlay();
                break;

            case ACTION_STARTDRAWING:
                replayAnnotations->startPenDraw(tagBytes[3]);
                break;

            case ACTION_STOPDRAWING:
                replayAnnotations->stopPenDraw(tagBytes[3]);
                break;

            case ACTION_REDO:
                replayAnnotations->redoPenAction(tagBytes[3]);
                break;

            case ACTION_UNDO:
                replayAnnotations->undoPenAction(tagBytes[3]);
                break;

            case ACTION_BRUSHFROMLAST:
                replayAnnotations->brushFromLast(tagBytes[3]);
                break;

            case ACTION_TRIANGLE_BRUSH:
                if( replayFile.read((char *)(4+tagBytes),4) < 0 )
                {
                    error = true;
                    break;
                }
                replayAnnotations->explicitBrush(tagBytes[3],SHAPE_TRIANGLE,
                                                 (tagBytes[4]<<8)+tagBytes[5],
                                                 (tagBytes[6]<<8)+tagBytes[7]);
                break;

            case ACTION_BOX_BRUSH:
                if( replayFile.read((char *)(4+tagBytes),4) < 0 )
                {
                    error = true;
                    break;
                }
                replayAnnotations->explicitBrush(tagBytes[3],SHAPE_BOX,
                                                 (tagBytes[4]<<8)+tagBytes[5],
                                                 (tagBytes[6]<<8)+tagBytes[7]);
                break;

            case ACTION_CIRCLE_BRUSH:
                if( replayFile.read((char *)(4+tagBytes),4) < 0 )
                {
                    error = true;
                    break;
                }
                replayAnnotations->explicitBrush(tagBytes[3],SHAPE_CIRCLE,
                                                 (tagBytes[4]<<8)+tagBytes[5],
                                                 (tagBytes[6]<<8)+tagBytes[7]);
                break;

            case ACTION_DOBRUSH:

                if( replayFile.read((char *)&(tagBytes[4]),4) < 0 )
                {
                    error = true;
                    break;
                }

                replayAnnotations->CurrentBrushAction(tagBytes[3],
                                                   (tagBytes[4]<<8)+tagBytes[5],
                                                   (tagBytes[6]<<8)+tagBytes[7]);
                break;

            case ACTION_PENPOSITIONS:

                ptr = tagBytes + 4;
                for( penNum=0; penNum<MAX_SCREEN_PENS; penNum++ )
                {
                    // Don't do the extra byte unless it's required
#if MAX_SCREEN_PENS > 8
                    if( ((tagBytes[2]<<8 | tagBytes[3]) & (1 << penNum)) )
#else
                    if( (tagBytes[3] & (1 << penNum)) )
#endif
                    {
                        if( replayFile.read((char *)ptr,4) < 0 )
                        {
                            error =true;
                            break;
                        }

                        xLoc[penNum] = (ptr[0]<<8) + ptr[1];
                        yLoc[penNum] = (ptr[2]<<8) + ptr[3];

                        replayPenCursor[penNum].move(xLoc[penNum],yLoc[penNum]);
                        replayPenCursor[penNum].show();
                        penPresent[penNum] = true;

                        ptr += 4;
                    }
                    else
                    {
                        replayPenCursor[penNum].hide();
                        penPresent[penNum] = false;
                    }
                }
                replayAnnotations->penPositions( penPresent, xLoc, yLoc );
                break;

            case ACTION_PENCOLOUR:
                if( replayFile.read((char *)tagBytes+4,4) < 0 )
                {
                    error =true;
                    break;
                }
                col    = QColor(tagBytes[4],tagBytes[5],tagBytes[6],tagBytes[7]);
                penNum = tagBytes[3];   // Legibility
                replayPenCursor[penNum].updatePenColour(col);
                qDebug() << " pen " << penNum << " colour " << col.name() << " ";
                replayAnnotations->setPenColour(penNum,col);
                break;

            case ACTION_PENWIDTH:

                qDebug() << " pen " << ((int)(tagBytes[3])) << " width " << ((int)(tagBytes[2])) << " ";
                replayAnnotations->setPenThickness(tagBytes[3],tagBytes[2]);
                break;

            case ACTION_PENTYPE:

                qDebug() << " pen " << ((int)(tagBytes[3])) << " type " << ((int)(tagBytes[2])) << " ";
                switch( tagBytes[2] )
                {
                case TYPE_DRAW:        replayAnnotations->penTypeDraw( tagBytes[3] );
                                       break;
                case TYPE_HIGHLIGHTER: replayAnnotations->penTypeHighlighter( tagBytes[3] );
                                       break;
                case TYPE_ERASER:      replayAnnotations->penTypeEraser( tagBytes[3] );
                                       break;
                }
                break;

            case ACTION_NEXTPAGE:

                replayAnnotations->nextPage();
                break;

            case ACTION_PREVPAGE:

                replayAnnotations->previousPage();
                break;

            case ACTION_GOTOPAGE:

                replayAnnotations->gotoPage((tagBytes[1]<<16)+(tagBytes[2]<<8)+tagBytes[3]);
                break;

            default:
                break;
            }
        }
    }

    if( error )
    {
        // Ensure our pens are no longer visible

        for(int p=0; p<MAX_SCREEN_PENS; p++)
        {
            replayPenCursor[p].hide();
        }
    }

    return error ? -1 : timeDiff;
}

void    replayManager::stopPlayback(void)
{
    qDebug("replayManager::stopPlayback");

    if( ! playingBack ) return;

    replayFile.close();

    // Just to be safe...
    replayAnnotations->setBackground(Qt::transparent);
    behindTheAnnotations->fill(Qt::transparent);

    // Ensure our pens are no longer visible

    for(int p=0; p<MAX_SCREEN_PENS; p++)
    {
        replayPenCursor[p].hide();
    }

    playingBack = false;
}


void    replayManager::rewindPlayback(void)
{
    qDebug("replayManager::rewindPlayback");

    if( ! playingBack ) return;

    replayFile.seek(0);

    // Copy the current replay pens from the base values
    replayAnnotations->setDefaults();

    replayAnnotations->imageData()->fill(Qt::transparent);
    behindTheAnnotations->fill(Qt::transparent);

    for(int i=0; i<sizeof(playbackTimepoint); i++ ) playbackTimepoint[i] = 0;

    // And playback until time point changes [ie draw starting point]
    nextPlaybackStep();
}



// ///// //
// ///// //
// DEBUG //
// ///// //
// ///// //

QString replayManager::decodeWords(quint32 wordIn)
{
    QString buf;
    int            numWordsRequired;
    static int     numWords = 0;
    static quint32 word[MAX_SCREEN_PENS+2];

    switch(ACTION_TYPE(word[0]))
    {
    case ACTION_TIMEPOINT:
        return QString("TIMEPOINT %1\n").arg(word[0]&0xFFFFFF);

    case ACTION_BACKGROUND_PNG:
        return QString("BACKGROUND PNG IMAGE (%1 bytes)\n").arg(word[0]&0xFFFFFF);

    case ACTION_BACKGROUND_JPG:
        return QString("BACKGROUND JPG IMAGE (%1 bytes)\n").arg(word[0]&0xFFFFFF);

    case ACTION_BACKGROUND_COLOUR:
        if( numWords == 0 )
        {
            word[0] = wordIn;
            numWords ++;
            return "";
        }
        else
        {
            word[numWords] = wordIn;
            buf = QString("[%1,%2,%3:%4").arg((word[1]>>24)&255)
                                         .arg((word[1]>>16)&255)
                                         .arg((word[1]>>8)&255)
                                         .arg(word[1]&255);
            numWords = 0;
            return QString("BACKGROUND COLOUR: ")+buf+QString("\n");
        }

    case ACTION_CLEAR_SCREEN:
        return QString("CLEAR OVERLAY");

    case ACTION_SCREENSIZE:
        if( numWords == 0 )
        {
            word[0] = wordIn;
            numWords ++;
            return "";
        }
        else
        {
            word[numWords] = wordIn;
            buf = QString("[%1,%2]").arg((word[1]>>16)&0xFFFF)
                                    .arg(word[1]&0xFFFF);
            numWords = 0;
            return QString("SCREEN SIZE: ")+buf+QString("\n");
        }

    case ACTION_STARTDRAWING:
        return QString("PEN: %1 START DRAWING\n").arg(word[0]&255);

    case ACTION_STOPDRAWING:
        return QString("PEN: %1 STOP DRAWING\n").arg(word[0]&255);

    case ACTION_PENPOSITIONS:

        numWordsRequired = 1;
        for(int p=1; p<=MAX_SCREEN_PENS; p++ )
        {
            if( (word[0] & (1 << p)) )
            {
                numWordsRequired ++;
            }
        }

        if( numWords == 0 )
        {
            word[0]  = wordIn;
            numWords = 0;
        }
        else if( numWords < numWordsRequired )
        {
            numWords ++;
            word[numWords] = wordIn;
        }
        else
        {
            buf="PEN POSNS: ";

            for(int p=1; p<=MAX_SCREEN_PENS; p++ )
            {
                if( (word[0] & (1 << p)) )
                {
                    buf += QString("%1 (%2,%3) ").arg(p-1)
                                                 .arg(((word[p]) >> 16 ) & 65535)
                                                 .arg(word[p] & 65535);
                }
            }
            return buf+QString("\n");
        }
        return "";

    case ACTION_PENCOLOUR:
        if( numWords == 0 )
        {
            word[0] = wordIn;
            numWords ++;
            return "";
        }
        else
        {
            word[numWords] = wordIn;
            buf = QString("[%1,%2,%3:%4").arg((word[1]>>24)&255)
                                         .arg((word[1]>>16)&255)
                                         .arg((word[1]>>8)&255)
                                         .arg(word[1]&255);
            numWords = 0;
            return QString("PEN: %1 COLOUR: ").arg(word[0]&255)+buf+QString("\n");
        }

    case ACTION_PENWIDTH:
        return QString("PEN: %1 WIDTH %2\n").arg(word[0]&255).arg((word[0]>>8)&65535);

    case ACTION_PENTYPE:
        buf = QString("PEN: %1 TYPE ").arg(word[0]&255);
        switch((word[0]>>8)&255)
        {
        case TYPE_DRAW:        return buf+"DRAW\n";
        case TYPE_HIGHLIGHTER: return buf+"HIGHLIGHTER\n";
        case TYPE_ERASER:      return buf+"ERASER\n";
        default:               return buf+QString("Invalid(%1)\n").arg((word[0]>>8)&255);
        }

    case ACTION_UNDO:
        return QString("PEN: %1: UNDO\n").arg(word[0]&255);

    case ACTION_REDO:
        return QString("PEN: %1: REDO\n").arg(word[0]&255);

    case ACTION_DOBRUSH:
        return QString("PEN: %1: DO BRUSH").arg(word[0]&255);


    default:
        return QString("Bad tag: %1\n").arg(word[0],1,16);
    }
}
