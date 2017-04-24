#ifndef SCREENGRABBER_H
#define SCREENGRABBER_H

#include <QWidget>
#include <QScreen>
#include <QPixmap>
#include <QThread>
#include <QVector>
#include <QMutex>
#include <QByteArray>

// Current 16x16. Power is used in message to allow variable sizes later
#define BG_IMG_TILE_POW   4
#define BG_IMG_TILE_SIZE (1<<BG_IMG_TILE_POW)
//#define BIG_TILE_FACTOR   8

//#define PNG_COMPRESS_SMALL_TILES

class screenGrabber : public QThread
{
    Q_OBJECT
public:
    explicit screenGrabber(QObject *inParent = 0);

    void             run();
    void             stopCommand(void);
    void             waitForReady(void);

    void             pauseGrabbing(void);   // Let's not broadcast if we are a client
    void             resumeGrabbing(void);

    QPixmap          currentBackground(void);
    int              nextSmallIndexesUpdateNum(int &imageIndexToUpdate);
    bool             getRepeatsAndUpdateIndex(int searchTile, QVector<int> &matchingTiles, QVector<int> &tileIndices);
    int              countOutOfDateSmallTiles(int &bigTileIndex, QVector<int> &currentTileUpdate, QVector<bool> &tileMask);
    QByteArray      *grabSmallTileData(int index, int &updateNum);
    void             releaseTileData(int index);
    void             screenCoordsOfSmallTile(int tileNum, int &x, int &y);
    void             nextBigTileIndexAndCoords(int &imageIndexToUpdate, int &updateNum, int &x, int &y);
    QByteArray      *grabBigTileData(int index, int &updateNum);
    void             addAUser(void);
    void             releaseAUser(void);
    void             updateScreenDimensions(int &width, int &height);

    bool             backgroundIsOpaque;    // Write from window or wherever. Let's us optomise.

    static const int numBigPerSmallPower     = 3;
    static const int numSmallTilesPerBigXorY = 8;

signals:
    
public slots:

private:

    void                  resizeArrays(int numBigTilesInX,   int backgroundHeight);
    bool                  smallTileIndexIsOnScreen(int smallTileNum);

    bool                  quitRequest;
    QWidget              *parent;

    QImage               *mainAnnotations;
    QImage               *mainHighlights;

    QPixmap               currentScreenSnap;  // This is updated in the thread
    QScreen              *screen;

    int                   backGroundWidth, backgroundHeight;
    int                   numSmallXTiles,  numSmallYTiles;
    int                   numBigTilesInX,  numBigTilesInY;
//    int                   numVirtualSmallXTiles;

    int                   visibleBuffer;
    int                   workingBuffer;
    bool                  haveData;

    QMutex               *access;
    QMutex               *pause;

    QVector<int>           repeatListNum[2];
    QVector<QVector<int> > repeatList[2];

    int                    screenGrabNumber;

    QVector<int>          lastSmallTileUpdateNum[2];
    QVector<QByteArray *> compressedSmallTile[2];
    QVector<bool>         tileChanged;
    QVector<bool>         tileChecked;

    QVector<int>          lastBigTileUpdateNum[2];
    QVector<QByteArray *> compressedBigTile[2];

    int                   userCount;
};

#endif // SCREENGRABBER_H
