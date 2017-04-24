#ifndef STICKYNOTE_H
#define STICKYNOTE_H

#include <QWidget>
#include <QVector>
#include <QPoint>
#include <QTimer>
#include <QTime>
#include <QRect>

#include "penStates.h"
#include "overlay.h"                  // For MAX_SCREEN_PENS mainly
//#include "handwritingRecognition.h"
//#include "textOverlay.h"
//
//#define MIN_TM_TO_LETTER_CHECK 5
//#define TEXT_STROKE_DELETE_TM 20
//
//typedef struct
//{
//    long            strokeID;
//    QVector<QPoint> point;    // List of points making this stroke
//    bool            complete;
//    long            addTime;  // Time (# msecs) at which stroke finished
//
//} stroke_t;

class stickyNote : public QWidget
{
    Q_OBJECT
public:
    explicit stickyNote(int height, int width, int penNum,
                        bool *penInTextModeRef, bool *brushActionRef,
                        QColor defaultPenColour[], int startThickness[],
                        QColor colour = Qt::yellow, QWidget *parent = 0);
    QRect    screenRect(void);
    void     applyPenData(int penNum, QPoint inPenPos, buttonsActions_t &buttons);
    void     penPositions(bool *present, int *xLoc, int *yLoc);
    void     setTextPosFromPixelPos(int penNum, int x, int y);
    void     addTextCharacterToOverlay(int penNum, quint32 charData);
    void     setNoteColour(int penNum, QColor newColour);
    void     penTypeDraw(int penNum);
    void     penTypeText(int penNum);
    void     penTypeEraser(int penNum);
    void     penTypeHighlighter(int penNum);
    void     setPenColour(int penNum, QColor newColour);
    bool     scaleStickySize(double scaleFactor);
    bool     clickIsInDragBar(QPoint checkPos);
    void     setStickyDragStart(QPoint startPos);
    void     dragToo(QPoint pos);

protected:
    void     paintEvent(QPaintEvent *ev);

signals:
    
public slots:

private:
    overlay             *annotations;
    QColor               backgroundColour;
    QImage              *annotationsImage;
    QImage              *highlights;
    QImage              *textLayerImage;

    QString             matches[6];         // Development of HWR only
    QImage              textOverlayDebug;

//    QVector<stroke_t *> stroke;             // All strokes made by all pens (deleted after 2 seconds)
//    QVector<stroke_t>   drawingStrokes;     // Not automatically deleted
    buttonsActions_t    lastPenButtons;
    QPoint              lastPos;
    int                 ownerPen;
//    long                nextStrokeID;
#ifndef Q_OS_ANDROID
//    responseDest_t      responses;          // Queue for responses from hwr thread
#endif
//    int                 lastHWRRequestTm;

//    QTimer              tmr;
//    long                tmrDeciSecs;

    QPoint              dragDeltaPos;
    QPoint              dragStartPoint;

    bool               *penInTextMode;
    bool               *brushAction;
    bool                waitForPenButtonsRelease[MAX_SCREEN_PENS];
    bool                penIsDrawing[MAX_SCREEN_PENS];

//    textOverlay        *text;

private slots:
//    void                     timerTick(void);

};

#endif // STICKYNOTE_H
