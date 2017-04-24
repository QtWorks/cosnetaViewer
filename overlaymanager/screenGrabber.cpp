#include <QApplication>
#include <QBuffer>
#include <QDesktopWidget>
#include "screenGrabber.h"

#include <QDebug>

screenGrabber::screenGrabber(QObject *inParent) : QThread(inParent)
{
    quitRequest        = false;
    backgroundIsOpaque = false;
    haveData           = false;

    parent = (QWidget *)inParent;

    screen = QGuiApplication::primaryScreen();

    qDebug() << "Start initial grab." << screen;

    // This is intended to be created when the main window is not in view. So grab the background now.
    currentScreenSnap = screen->grabWindow( QApplication::desktop()->winId() );

    qDebug() << "Initial grab complete.";

    backGroundWidth   = QApplication::desktop()->width();
    backgroundHeight  = QApplication::desktop()->height();

    access = new QMutex;
    pause  = new QMutex;

    qDebug() << "Screengrabber size grab to:" << backGroundWidth <<backgroundHeight;

    resizeArrays( backGroundWidth,backgroundHeight );

    lastSmallTileUpdateNum[0].fill(0);
    lastSmallTileUpdateNum[1].fill(0);

    lastBigTileUpdateNum[0].fill(0);
    lastBigTileUpdateNum[1].fill(0);

    workingBuffer = 1;
    visibleBuffer = 0;
    userCount     = 0;

    screenGrabNumber = 0;

    // Set previous screen grab to null
    currentScreenSnap = QPixmap();
}

void screenGrabber::addAUser(void)
{
    userCount ++;
}

void screenGrabber::waitForReady(void)
{
    while( ! haveData ) QThread::wait(50);

    return;
}

void screenGrabber::releaseAUser(void)
{
    if(userCount>0) userCount --;
}

void screenGrabber::stopCommand(void)
{
    quitRequest = true;
}

void screenGrabber::screenCoordsOfSmallTile(int tileNum, int &x, int &y)
{
    x = (tileNum % numSmallXTiles) * BG_IMG_TILE_SIZE;
    y = (tileNum / numSmallXTiles) * BG_IMG_TILE_SIZE;
}

void screenGrabber::resizeArrays(int backGroundWidth, int backgroundHeight)
{
    numBigTilesInX = (backGroundWidth+BG_IMG_TILE_SIZE*numSmallTilesPerBigXorY-1)  / (BG_IMG_TILE_SIZE*numSmallTilesPerBigXorY);
    numBigTilesInY = (backgroundHeight+BG_IMG_TILE_SIZE*numSmallTilesPerBigXorY-1) / (BG_IMG_TILE_SIZE*numSmallTilesPerBigXorY);

//    numSmallXTiles = (backGroundWidth+BG_IMG_TILE_SIZE-1)  / BG_IMG_TILE_SIZE;
//    numSmallYTiles = (backgroundHeight+BG_IMG_TILE_SIZE-1) / BG_IMG_TILE_SIZE;
    numSmallXTiles = numBigTilesInX*numSmallTilesPerBigXorY;
    numSmallYTiles = numBigTilesInY*numSmallTilesPerBigXorY;

//    numVirtualSmallXTiles = numBigTilesInX*numSmallTilesPerBigXorY;

    int  bigTileSize      = numBigTilesInX*numBigTilesInY;
    int  newSmallTileSize = numSmallXTiles*numSmallYTiles;

    qDebug() << "screenGrabber::resizeArrays to" <<newSmallTileSize;

    // Wait for any users of the higher indexes to stop being used

    int oldSize        = lastSmallTileUpdateNum[0].size();
    int oldBigTileSize = lastBigTileUpdateNum[0].size();

    lastSmallTileUpdateNum[0].resize(newSmallTileSize);
    lastSmallTileUpdateNum[1].resize(newSmallTileSize);
    compressedSmallTile[0].resize(newSmallTileSize);
    compressedSmallTile[1].resize(newSmallTileSize);
    lastBigTileUpdateNum[0].resize(bigTileSize);
    lastBigTileUpdateNum[1].resize(bigTileSize);
    compressedBigTile[0].resize(bigTileSize);
    compressedBigTile[1].resize(bigTileSize);
    repeatListNum[0].resize(newSmallTileSize);
    repeatListNum[1].resize(newSmallTileSize);
    repeatList[0].resize(newSmallTileSize);
    repeatList[1].resize(newSmallTileSize);
    tileChanged.resize(newSmallTileSize);
    tileChecked.resize(newSmallTileSize);

    for(int i=oldSize; i<newSmallTileSize; i++)
    {
        lastSmallTileUpdateNum[0][i]         = 0;
        lastSmallTileUpdateNum[1][i]         = 0;
        compressedSmallTile[0][i]            = new QByteArray;
        compressedSmallTile[1][i]            = new QByteArray;
        tileChanged[i]                       = true;
        tileChecked[i]                       = false;
    }

    for(int i=oldBigTileSize; i<bigTileSize; i++)
    {
        lastBigTileUpdateNum[0][i] = 0;
        lastBigTileUpdateNum[1][i] = 0;
        compressedBigTile[0][i]    = new QByteArray;
        compressedBigTile[1][i]    = new QByteArray;
    }
}


// This is the worker thread. Here we:
//  * grab the current desktop
//  * mask out updates that aren't from freeStyle (leave them as currentScreenSnap)
//  * encode this as PNG tiles
//  * look for differences, incrementing an index for those that have changed.
// Wait, rinse, repeat...

void screenGrabber::run(void)
{
    QPixmap      latestGrab;
    QPixmap      tile(BG_IMG_TILE_SIZE,BG_IMG_TILE_SIZE);
    QPixmap      oldTile(BG_IMG_TILE_SIZE,BG_IMG_TILE_SIZE);
    int          x_origin, y_origin;

    qDebug() << "screenGrabber::run()";

    while( ! quitRequest )
    {
        if( backGroundWidth  != QApplication::desktop()->width()  ||
            backgroundHeight != QApplication::desktop()->height()   )
        {
            qDebug() << "Screengrabber: resize desktop to" << QApplication::desktop()->width() << QApplication::desktop()->height();

            // Record new size
            backGroundWidth   = QApplication::desktop()->width();
            backgroundHeight  = QApplication::desktop()->height();

            resizeArrays( backGroundWidth,backgroundHeight );

            tileChanged.fill(true);
            tileChecked.fill(false);
        }

        // Wait if no clients...
        if( userCount == 0 )
        {
            qDebug() << "Screengrabber: wait for a user thread...";

            while( userCount == 0 )
            {
                QThread::msleep(10);
            }

            qDebug() << "Screen grabber started...";
        }

        // Grab screen. TODO: grabWindow(WId window, int x = 0, int y = 0, int width = -1, int height = -1)
        latestGrab = screen->grabWindow( QApplication::desktop()->winId() );

        screenGrabNumber ++;    // TODO: how do we handle a wrap?

        // ///////////
        // SMALL TILES
        // ///////////

        // Update currentScreenSnap so it only has changes from the background
        // TODO: also need to hide all the cursors...

        for(int t=0; t<tileChanged.size(); t++)
        {
            x_origin = (t % numSmallXTiles) * BG_IMG_TILE_SIZE;
            y_origin = (t / numSmallXTiles) * BG_IMG_TILE_SIZE;

            if( x_origin < backGroundWidth && y_origin < backgroundHeight )
            {
                oldTile = currentScreenSnap.copy(x_origin,y_origin, BG_IMG_TILE_SIZE,BG_IMG_TILE_SIZE);
                tile    = latestGrab.copy(x_origin,y_origin, BG_IMG_TILE_SIZE,BG_IMG_TILE_SIZE);

                // latestGrab is our current best guess at the background. Encode and
                // store the latest PNG compressed tiles.

                if( oldTile.toImage() != tile.toImage() )
                {
#ifdef PNG_COMPRESS_SMALL_TILES
                    QBuffer    buffer( compressedSmallTile[workingBuffer][t] );
                    buffer.open(QIODevice::WriteOnly);
                    tile.save(&buffer, "PNG");

                    lastSmallTileUpdateNum[workingBuffer][t] = lastSmallTileUpdateNum[visibleBuffer][t] + 1;
#else
                    QImage tileImage = tile.toImage();

                    if( tileImage.format() != QImage::Format_RGB888 )
                    {
                        tileImage = tileImage.convertToFormat(QImage::Format_RGB888, Qt::ColorOnly);
                    }
#if Q_BYTE_ORDER == Q_BIG_ENDIAN
                    // NB Android, iOS etc use little endian mode on ARM, so that is the
                    // mode we transmit data in, on the basis that we want less work to be
                    // required there as they are slower processors.
                    QByteArray line;
#error "Not coded as my information suggests it's not gonna be relevant. Evfer."
#else
                    QByteArray imageData;

                    // NB We are assuming that space exists at the end of the line so the next
                    //    scanline is 32 bit aligned (this is implied in the QImage() creation functions.
                    int lineLen = (tileImage.bytesPerLine()+3) & (~3);

                    for( int line=0; line<tileImage.height(); line++ )
                    {
                        imageData.append((const char *)tileImage.scanLine(line),lineLen);
                    }
#endif
                    // TODO: check for a memory leak here.
                    compressedSmallTile[workingBuffer][t]    = new QByteArray(qCompress(imageData,9));
                    lastSmallTileUpdateNum[workingBuffer][t] = screenGrabNumber;
#endif
                }
                else // Copy the old tile
                {
                    compressedSmallTile[workingBuffer][t]     = compressedSmallTile[visibleBuffer][t];
                    lastSmallTileUpdateNum[workingBuffer][t]  = lastSmallTileUpdateNum[visibleBuffer][t];
                }
            }
        }

        // Check for repeats (in common screenGrabber thread as used by all clients)
        tileChecked.fill(false);
        for(int t=0; t<tileChanged.size(); t++)
        {
            repeatList[workingBuffer][t].clear();

            // Only check this if it's not in another set
            if( ! tileChecked[t] && smallTileIndexIsOnScreen(t) )
            {
                tileChecked[t] = true;

                repeatListNum[workingBuffer][t] = -1;   // Flag that we're using our own list
                repeatList[workingBuffer][t].append(t); // Add ourself

                for( int chk=t+1; chk<tileChanged.size(); chk ++ )
                {
                    // Don't need to check if it's in another set
                    if( ! tileChecked[chk] && smallTileIndexIsOnScreen(chk) )
                    {
                        if( *compressedSmallTile[workingBuffer][t] == *compressedSmallTile[workingBuffer][chk] )
                        {
//                            qDebug() << "Tile" << t << "matches" << chk;
                            repeatList[workingBuffer][t].append(chk);
                            repeatListNum[workingBuffer][chk] = t;   // So no it knows where the list is
                            tileChecked[chk] = true;
                        }
                    }
                }
            }
        }

        // /////////
        // BIG TILES
        // /////////

        for(int bigT=0; bigT<lastBigTileUpdateNum[0].size(); bigT++)
        {
//            QString changeDebug = QString("Big Tiles %1. Small tiles: ").arg(bigT);

            // Have any of the small tiles changed ?
            bool  changed = false;
            int   xInBigTiles = (bigT % numBigTilesInX);
            int   yInBigTiles = (bigT / numBigTilesInX);

            // Changed ?

            for(int dy=0; dy<numSmallTilesPerBigXorY && ! changed; dy++)
            {
                int lineStart = (dy + yInBigTiles*numSmallTilesPerBigXorY)*numSmallXTiles +
                                      xInBigTiles*numSmallTilesPerBigXorY;

                for(int dx=0; dx<numSmallTilesPerBigXorY; dx++)
                {
                    int smallT = lineStart + dx;

                    if( lastSmallTileUpdateNum[workingBuffer][smallT] !=
                        lastSmallTileUpdateNum[visibleBuffer][smallT]    )
                    {
                        changed = true;
                        break;
                    }
                }
            }

            // Yup. Generate a new tile
            if( changed )
            {
                x_origin = xInBigTiles * BG_IMG_TILE_SIZE*numSmallTilesPerBigXorY;
                y_origin = yInBigTiles * BG_IMG_TILE_SIZE*numSmallTilesPerBigXorY;

                tile = latestGrab.copy(x_origin,y_origin,
                                       BG_IMG_TILE_SIZE*numSmallTilesPerBigXorY,
                                       BG_IMG_TILE_SIZE*numSmallTilesPerBigXorY);

                QBuffer    buffer( compressedBigTile[workingBuffer][bigT] );
                buffer.open(QIODevice::WriteOnly);
                if( ! tile.save(&buffer, "JPG",12) )
                {
                    qDebug() << "Failed to compress bigTile" << bigT;
                }
                else
                {
//                    qDebug() << "Made big tile" << bigT << "change data:" << changeDebug;
                }

                lastBigTileUpdateNum[workingBuffer][bigT] = screenGrabNumber;
            }
            else // No change
            {
                compressedBigTile[workingBuffer][bigT]    = compressedBigTile[visibleBuffer][bigT];
                lastBigTileUpdateNum[workingBuffer][bigT] = lastBigTileUpdateNum[visibleBuffer][bigT];
            }
        }
//qDebug() << "Big Indices:" << lastBigTileUpdateNum[workingBuffer];

        // Keep for next comparison
        currentScreenSnap = latestGrab;

        // Wait till no-one is using our buffers & swap
        access->lock();
        if( visibleBuffer == 0 )
        {
            visibleBuffer = 1;
            workingBuffer = 0;
        }
        else
        {
            visibleBuffer = 0;
            workingBuffer = 1;
        }
        access->unlock();

        if( ! haveData )
        {
            qDebug() << "Have screen grab data to send.";
            haveData = true;
        }

        // Allow for pausing the grabber task
#if 0
        pause->lock();
        pause->unlock();
#endif
        QThread::msleep(10);
//            msleep(100);    // TODO: calculate delay so that overall rate tends to 4 Hz
    }
}

void screenGrabber::pauseGrabbing(void)
{
    qDebug() << "Pause screengrabber";
// TODO: manage this
    pause->lock();
}

void screenGrabber::resumeGrabbing(void)
{
    qDebug() << "Re-start screengrabber";
// TODO: manage this
    pause->unlock();
}



bool screenGrabber::smallTileIndexIsOnScreen(int smallTileNum)
{
    int x = (smallTileNum % numSmallXTiles) * BG_IMG_TILE_SIZE;
    int y = (smallTileNum / numSmallXTiles) * BG_IMG_TILE_SIZE;

    return (x < backGroundWidth) && (y < backgroundHeight);
}



QPixmap screenGrabber::currentBackground(void)
{
    if( userCount == 0 )
        return screen->grabWindow( QApplication::desktop()->winId() );
    else
        return currentScreenSnap;
}


void screenGrabber::updateScreenDimensions(int &width, int &height)
{
    width  = backGroundWidth;
    height = backgroundHeight;
}


int screenGrabber::nextSmallIndexesUpdateNum(int &imageIndexToUpdate)
{
    imageIndexToUpdate ++;
#if 1
    if( ((imageIndexToUpdate / numSmallXTiles) * BG_IMG_TILE_SIZE)>=backgroundHeight )
    {
        imageIndexToUpdate = 0;
    }
    // NB Not dealing with off the right edge yet as this just means the tile grab fails and retries
#else
    if( imageIndexToUpdate>=lastSmallTileUpdateNum[visibleBuffer].size() ) imageIndexToUpdate = 0;
#endif
    return lastSmallTileUpdateNum[visibleBuffer][imageIndexToUpdate];
}


void screenGrabber::nextBigTileIndexAndCoords(int &imageIndexToUpdate, int &updateNum, int &x, int &y)
{
    imageIndexToUpdate ++;
    if( imageIndexToUpdate>=lastBigTileUpdateNum[visibleBuffer].size() ) imageIndexToUpdate = 0;

    x = ((imageIndexToUpdate % numBigTilesInX)*BG_IMG_TILE_SIZE*numSmallTilesPerBigXorY);
    y = ((imageIndexToUpdate / numBigTilesInX)*BG_IMG_TILE_SIZE*numSmallTilesPerBigXorY);
    updateNum = lastBigTileUpdateNum[visibleBuffer][imageIndexToUpdate];
}



QByteArray *screenGrabber::grabSmallTileData(int index, int &updateNum)
{
    access->lock();

    int bufNum = visibleBuffer;

    if( index>=compressedSmallTile[bufNum].size() || index<0 )
    {
        qDebug() << "HACK to carry on: return index 0 instead of" << index;

        // This could happen if the screen is resized
        index = 0;
    }

    updateNum = lastSmallTileUpdateNum[bufNum][index];

    QByteArray *dat = compressedSmallTile[bufNum][index];

    access->unlock();

    return dat;
}


QByteArray *screenGrabber::grabBigTileData(int index, int &updateNum)
{
    access->lock();

    int bufNum = visibleBuffer;

    if( index>=compressedBigTile[bufNum].size() || index<0 )
    {
        qDebug() << "HACK to carry on: return index 0 instead of" << index;

        // This could happen if the screen is resized
        index = 0;
    }

    updateNum = lastBigTileUpdateNum[bufNum][index];

    QByteArray *dat = compressedBigTile[bufNum][index];

    access->unlock();

    return dat;
}


void screenGrabber::releaseTileData(int index)
{
//    if( index>=compressedSmallTile[visibleBuffer].size() )
//    {
//        qDebug() << "Tried to unlock index" << index;
//    }
}


bool screenGrabber::getRepeatsAndUpdateIndex(int           searchTile,
                                             QVector<int> &matchingTiles,
                                             QVector<int> &tileIndices)
{
    if( searchTile<0 || searchTile>=repeatListNum[visibleBuffer].size() )
    {
        qDebug() << "screenGrabber::getRepeatsAndUpdateIndex() bad index:" << searchTile;
        return false;
    }

    access->lock();

    if( repeatListNum[visibleBuffer][searchTile] < 0 )
    {
        matchingTiles = repeatList[visibleBuffer][searchTile];
    }
    else
    {
        matchingTiles = repeatList[visibleBuffer][repeatListNum[visibleBuffer][searchTile]];
    }

    tileIndices.clear();

    for(int i=0; i<matchingTiles.size(); i++)
    {
        tileIndices.append( lastSmallTileUpdateNum[visibleBuffer][matchingTiles[i]] );
    }
    access->unlock();

    // Return true if found any repats
    return tileIndices.size()>1;
}

int screenGrabber::countOutOfDateSmallTiles(int           &bigTileIndex,
                                            QVector<int>  &currentTileUpdate,
                                            QVector<bool> &tileMask)
{
    int    outOfDateCount = 0;
    int    xBig = (bigTileIndex % numBigTilesInX);
    int    yBig = (bigTileIndex / numBigTilesInX);

    tileMask.clear();

    int xStart = xBig*numSmallTilesPerBigXorY;
    int yStart = yBig*numSmallTilesPerBigXorY;

    for(int yChk=yStart; yChk < (yStart+numSmallTilesPerBigXorY); yChk ++)
    {
        for(int xChk=xStart; xChk < (xStart+numSmallTilesPerBigXorY); xChk ++)
        {
            int index = xChk + yChk*numSmallXTiles;

            if( smallTileIndexIsOnScreen(index)  &&  index < currentTileUpdate.size() )
            {
                if( lastSmallTileUpdateNum[visibleBuffer][index] != currentTileUpdate[index] )
                {
                    outOfDateCount ++;
                    tileMask.append(true);
                }
                else
                {
                    tileMask.append(false);
                }
            }
            else tileMask.append(false);
        }
    }

    return outOfDateCount;
}

