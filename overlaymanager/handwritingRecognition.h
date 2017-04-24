#ifndef HANDWRITINGRECOGNITION_H
#define HANDWRITINGRECOGNITION_H

#include <QVector>
#include <QPoint>
#include <QObject>
#include <QQueue>
#include <QChar>

#include "NNRecogniser.h"
#include "visionobjectsapi.h"
#include "vorecognitionresult.h"

// sent to requestor class
typedef struct
{
    QChar  ch;
    float  confidence;

} hwrSingleResult_t;

/**
typedef struct
{
    QString             shapeCandidateType;
    QString             shapeName;
    float               confidence;
    QVector<QString>    primitiveType;
    QVector<QPoint>     firstPoint;
    QVector<QPoint>     lastPoint;

} shapeSingleResult_t;

typedef struct
{
    QVector<shapeSingleResult_t> shapeNames;

} shapeResult_t;
**/

typedef struct
{
    QVector<hwrSingleResult_t>  match;
    int                         minX, maxX;
    int                         minY, maxY;
    int                         lastStrokeTm;    // Indicates consumed strokes to sender. It then deletes them.

    QVector<shapeResult_t>      shapeList;       // JF Added shapeList to carry back shape data (ugly)

} hwrResult_t;


typedef struct
{
    QString  hwrString;
    float  confidence;

} hwrSingleWordResult_t;

typedef struct
{
    QVector<hwrSingleWordResult_t> wordMatch;
    int                       minX, maxX;
    int                       minY, maxY;
    int                       lastStrokeTm;    // Indicates consumed strokes to sender. It then deletes them.
} hwrWordResult_t;

typedef QQueue<hwrResult_t> responseDest_t;

// Sent to this class
typedef struct
{
    responseDest_t           *answerQueue;  // Who to respond to. Void to avoid circular definition dependancies (yuk).
    int                       lastStrokeTm;
    QVector<QVector<QPoint> > strokePoints; // Each stroke is a vector of points

} hwrRequest_t;

// ////////////////////////////////////////
// Reference to global HWR request queue
// This is used both for the interface and
// internally in this class.
// ///////////////////////////////////////
extern QQueue<hwrRequest_t> *requestQueue;


// And the class definition

class handwritingRecognition : public QObject
{
    Q_OBJECT
public:
    explicit handwritingRecognition(QObject *parent = 0);

    void stopCommand(void);
    bool readyForRequests(void);
    void voStart(QVector<QVector<QPoint> > strokePoints);

signals:
    
public slots:
    void recogniserThread();
    void voRecogniserThread();
    void doConnect(QThread &cThread);
    void voShowResult( const QByteArray &resultsData);

private:
    bool                running;
    bool                ready;
    NNRecogniser        *shapeRecog;
    VoRecognitionResult *voShapeRecog;
    VisionObjectsAPI    m_api;
};

#endif // HANDWRITINGRECOGNITION_H
