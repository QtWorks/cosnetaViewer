#ifndef REPLAYMANAGER_H
#define REPLAYMANAGER_H

#include <QObject>
#include <QString>
#include <QTime>

#include "viewServer.h"
#include "overlay.h"

#define STRINGIFY(A) #A

// This object wraps around an overlay object to allow the display of
// a recorded overlay. It uses the same API as immediate mode and also
// reads/saves to files and adds time information to the saved stream.
// It also requires constant updates from the pen locations.

class replayManager : public QWidget
{
    Q_OBJECT

public:
    explicit replayManager(QImage *drawImage,  QImage  *highlightImage, QImage *txtImage,
                           QColor *initColour, QWidget *parent = 0 );
    
    void    setBackground(QColor bgColour);
    QImage *imageData(void);

    void    setPenColour(int penNum, QColor col);
    void    setPenThickness(int penNum, int thick);

    void    startPenDraw(int penNum);
    void    stopPenDraw(int penNum);

    void    penTypeDraw(int penNum);
    void    penTypeText(int penNum);
    void    penTypeHighlighter(int penNum);
    void    penTypeEraser(int penNum);

    void    nextPage(void);
    void    previousPage(void);
    void    gotoPage(int number);
    void    clearOverlay(void);

    void    setTextPosFromPixelPos(int penNum, int x, int y);
    void    addTextCharacterToOverlay(int penNum, quint32 charData);

    bool    saveDiscussion(QString fileName);
    void    finishUpSaving(void);
    bool    currentlyRecording(void);
    bool    currentlyPlaying(void);

    bool    replayDiscussion(QString fileName, QImage *background);
    int     nextPlaybackStep(void);             // Returns msec till next change
    void    stopPlayback(void);
    void    rewindPlayback(void);

    void    penPositions(bool newPenPresent[], int xLoc[], int yLoc[]);

    void    undoPenAction(int penNum);
    void    redoPenAction(int penNum);
    void    repeatPenAction(int penNum, int x, int y);
    void    brushFromLast(int penNum);
    void    explicitBrush(int penNum, shape_t shape, int param1, int param2);
    void    zoomBrush(int penNum, int scalePercent);


private:
    void    loadReplayBackgroundPNGImage(int length);
    void    loadReplayBackgroundJPGImage(int length);
    void    writeTimePointToFile(void);
    QString decodeWords(quint32 wordIn);

    overlay            *replayAnnotations;
    penCursor           replayPenCursor[MAX_SCREEN_PENS];

    QImage             *behindTheAnnotations;
    int                 w;
    int                 h;

    bool                recording;
    bool                playingBack;

    QFile               replayFile;
    unsigned char       playbackTimepoint[4];

    QFile               saveFile;
    QDataStream         saveStream;
    unsigned char       lastSaveTimepoint[4];

    bool                penPresent[MAX_SCREEN_PENS];
    int                 penPosPrevX[MAX_SCREEN_PENS];
    int                 penPosPrevY[MAX_SCREEN_PENS];

    QColor              penColour[MAX_SCREEN_PENS];       // Allows save state to be saved
    int                 penThickness[MAX_SCREEN_PENS];
    int                 penDrawstyle[MAX_SCREEN_PENS];

    QTime               sessionTime;
};

#endif // REPLAYMANAGER_H
