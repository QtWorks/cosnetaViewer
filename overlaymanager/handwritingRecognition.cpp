#include "handwritingRecognition.h"
#include "NNRecogniser.h"
#include "visionobjectsapi.h"
#include "vorecognitionresult.h"

#include <QThread>
#include <QDebug>

#define USE_VO false // Set to true to use the Vision Objects MyScript Web Service

// Create static table of unicode values for each symbol (from unicodeMapfile_alphanumeric.ini)

#define NUM_SYMBOLS 72

static QChar unicodeCharsFromSymbolNum[NUM_SYMBOLS] =
{
    0x0041,0x0042,0x0043,0x0044, 0x0045,0x0046,0x0047,0x0048,
    0x0049,0x004A,0x004B,0x004C, 0x004D,0x004E,0x004F,0x0050,
    0x0051,0x0052,0x0053,0x0054, 0x0055,0x0056,0x0057,0x0058,
    0x0059,0x005A,0x0061,0x0062, 0x0063,0x0064,0x0065,0x0066,
    0x0067,0x0068,0x0069,0x006A, 0x006B,0x006C,0x006D,0x006E,
    0x006F,0x0070,0x0071,0x0072, 0x0073,0x0074,0x0075,0x0076,
    0x0077,0x0078,0x0079,0x007A, 0x0030,0x0031,0x0032,0x0033,
    0x0034,0x0035,0x0036,0x0037, 0x0038,0x0039,0x002E,0x002C,
    0x003F,0x002D,0x0040,0x003A, 0x003B,0x0028,0x0029,0x002B
};

handwritingRecognition::handwritingRecognition(QObject *parent) : QObject(parent)
{
    ready    = false;
    running  = true;
}

void handwritingRecognition::doConnect(QThread &cThread)
{
    if (!USE_VO)
    {
        connect(&cThread, SIGNAL(started()), this, SLOT(recogniserThread()) );
    }
    else
    {
        connect(&cThread, SIGNAL(started()), this, SLOT(voRecogniserThread()) );
    }
}

void handwritingRecognition::stopCommand(void)
{
    ready   = false;
    running = false;     // Don't detach as it's ours, and we may be restarted.
}

static int debugCount = 0;

void handwritingRecognition::recogniserThread()
{
    qDebug() << "Handwriting recognition thread started.";

    // Load the handwiting recognition data (slow)
    shapeRecog = new NNRecogniser();

    shapeRecog->loadModelData("nn.mdt");

    // Now we are ready to recognise some symbols

    ready = true;

    while( running )
    {
        if( ! requestQueue->isEmpty() )
        {
            hwrRequest_t req = requestQueue->dequeue();

            qDebug() << "Received handwriting request with" << req.strokePoints.size() << "strokes.";

            hwrResult_t       result;

            if( req.strokePoints.size() > 0 )
            {
                // Try and recognise something
                trace_t              traceSet;
                shapeRecoResultSet_t results;
                tracePoint_t         point;
                traceLine_t          trl;

                result.minX = req.strokePoints[0][0].x();
                result.maxX = req.strokePoints[0][0].x();
                result.minY = req.strokePoints[0][0].y();
                result.maxY = req.strokePoints[0][0].y();

                for(int s=0; s<req.strokePoints.size(); s++)
                {
                    traceSet.line.push_back(trl);
                    for(int p=0; p<req.strokePoints[s].size(); p++)
                    {
                        point.X = req.strokePoints[s][p].x();
                        point.Y = req.strokePoints[s][p].y();

                        if( result.minX > point.X ) result.minX = point.X;
                        if( result.maxX < point.X ) result.maxX = point.X;
                        if( result.minY > point.Y ) result.minY = point.Y;
                        if( result.maxY < point.Y ) result.maxY = point.Y;

                        traceSet.line[s].push_back(point);

                        trl.clear();
                    }
                }

                hwrSingleResult_t entry;
                result.match.clear();

                if( shapeRecog->recognise(traceSet, results) )
                {
                    qDebug() << "recognise complete:" << results.size() << "Returned";

                    for(int r=0; r<results.size(); r++)
                    {
                        if( results[r].shapeID >= 0 && results[r].shapeID < NUM_SYMBOLS )
                        {
                            entry.ch         = unicodeCharsFromSymbolNum[results[r].shapeID];
                            entry.confidence = results[r].confidence;

                            result.match.push_back(entry);
                        }
                        else
                        {
                            qDebug() << "Unexpected symbol match:" << results[r].shapeID;
                        }
                    }
                }
                else
                {
                    qDebug() << "recognise failed:";
                }
                result.lastStrokeTm = req.lastStrokeTm;
            }

            qDebug() << "Num results:" << result.match.size();

            req.answerQueue->enqueue(result);

            qDebug() << "Responded to handwriting request.";
        }
        else
        {
            // Wait for 1/10 sec (the cycle time in stickies)
            QThread::msleep(100);
        }
    }

    qDebug() << "handwritingRecognition thread exiting";
}

void handwritingRecognition::voRecogniserThread()
{
    voShapeRecog = new VoRecognitionResult();
    qDebug() << "Vision Objects handwriting recognition thread started.";

    // Now we are ready to recognise some symbols
    ready = true;

    while( running )
    {
        if( ! requestQueue->isEmpty() )
        {
            hwrRequest_t req = requestQueue->dequeue();

            qDebug() << "Received handwriting request with" << req.strokePoints.size() << "strokes.";

            hwrResult_t  result;

            if( req.strokePoints.size() > 0 )
            {
                // start the VO recognition process
                voStart(req.strokePoints);

                // Try and recognise something
                trace_t              traceSet;
                //shapeRecoResultSet_t results;
                tracePoint_t         point;
                traceLine_t          trl;

                result.minX = req.strokePoints[0][0].x();
                result.maxX = req.strokePoints[0][0].x();
                result.minY = req.strokePoints[0][0].y();
                result.maxY = req.strokePoints[0][0].y();

                for(int s=0; s<req.strokePoints.size(); s++)
                {
                    traceSet.line.push_back(trl);
                    for(int p=0; p<req.strokePoints[s].size(); p++)
                    {
                        point.X = req.strokePoints[s][p].x();
                        point.Y = req.strokePoints[s][p].y();

                        if( result.minX > point.X ) result.minX = point.X;
                        if( result.maxX < point.X ) result.maxX = point.X;
                        if( result.minY > point.Y ) result.minY = point.Y;
                        if( result.maxY < point.Y ) result.maxY = point.Y;

                        traceSet.line[s].push_back(point);

                        trl.clear();
                    }
                }

                // Get textLines
                if (voShapeRecog->textLines().size() > 0)
                {
                    QStringList foundWordList = voShapeRecog->textLines();
                    QString foundWords = foundWordList.join(" "); // String the textLines together

                    hwrSingleResult_t symbol;
                    result.match.clear();
                    // Loop over all the characters in the foundWords string
                    for (int i = 0; i < foundWords.size(); ++i)
                    {
                        symbol.ch = foundWords.at(i);
                        symbol.confidence = 1.0; //TODO take this from resemblanceScore
                        result.match.push_back(symbol);
                    }
                }

                // Get shapes
                qDebug() << "handwritingRecognition.cpp: All shape candidates found are " << voShapeRecog->shapes();
                qDebug() << "handwritingRecognition.cpp: Number of shapeResults stored is " << voShapeRecog->sizeShapeResults();
                if (voShapeRecog->shapes().size() > 0)
                {
                    shapeResult_t shape;
                    shapeSingleResult_t shapeCandidate;

                    //QStringList foundShapesList = voShapeRecog->shapes(); // This gives all the shape candidates for each shape - just used for info
                    //QString foundShapes = foundShapesList.at(0); // Just take the first shape for now

                    // Loop over the collection of shapes found in the vector of shapeResult_t's as stored in result.shapeList
                    // Each shape in shapeResult_t has a set of shape candidates with attributes defined by shapeSingleResult_t
                    /**
                    QVectorIterator<shapeResult_t> shapeResultIter(result.shapeList);
                    while (shapeResultIter.hasNext())
                    {
                        shape = result.shapeList(*shapeResultIter);
                        // Now iterate over the shapes picking out each of the shapeCandidates
                        QVectorIterator<shapeSingleResult_t> shapeCandidateIter(shape);
                        while (shapeCandidateIter.hasNext())
                        {
                            shapeCandidate = shape(*shapeCandidateIter);
                            qDebug() << "shapeCandidate.shapeName is " << shapeCandidate.shapeName;
                            shapeCandidateIter.next();
                        }
                        shapeResultIter.next();
                    }
                    **/

                    qDebug() << "*************************";
                    qDebug() << "result.shapeList.size() is " << result.shapeList.size();

                    QVector<shapeResult_t>::iterator shapeResultIter;
                    int i = 0, j = 0;

                    for (shapeResultIter = result.shapeList.begin(); shapeResultIter != result.shapeList.end(); shapeResultIter++)
                    {
                        shape = result.shapeList.at(i);

                        qDebug() << i << " shape.shapeNames.size() is " << shape.shapeNames.size();

                        /**
                        j = 0;
                        QVector<shapeSingleResult_t>::iterator shapeCandidatesIter;
                        for (shapeCandidatesIter = shape.shapeNames.begin(); shapeResultIter != shape.shapeNames.end(); shapeResultIter++)
                        {
                            shapeCandidate = shape.shapeNames.at(j);
                            qDebug() << "   " << j << " shapeCandidate is " << shapeCandidate.shapeName;
                            ++j;
                        }
                        ++i; // Next shape please
                        **/
/**
                        typedef struct
                        {
                            QVector<shapeSingleResult_t> shapeNames;

                        } shapeResult_t;

                        typedef struct
                        {
                            QString             shapeCandidateType;
                            QString             shapeName;
                            float               confidence;
                            QVector<QString>    primitiveType;  // a shape is made up of primitives such as lines ("line" being a primitiveType)
                            QVector<QPoint>     firstPoint;     // e.g. a triangle is 3 lines
                            QVector<QPoint>     lastPoint;      // and each line has a firstPoint and a lastPOint

                        } shapeSingleResult_t;
**/
                        // Now iterate over the shapes picking out each of the shapeCandidates

                        //QVector<shapeSingleResult_t>::iterator shapeCandidateIter;
                        //for (shapeCandidateIter = result.shapeList.begin(); shapeCandidateIter != result.shapeList.end(); shapeCandidateIter++)
                        //{

                        //while (shapeCandidateIter.hasNext())
                        //{
                        //    shapeCandidate = shape(*shapeCandidateIter);
                        //    qDebug() << "shapeCandidate.shapeName is " << shapeCandidate.shapeName;
                        //    shapeCandidateIter.next();
                        //}
                        //shapeResultIter.next();
                    }


                    /**
                    for (int i = 0; i < result.shapeList.size(); ++i)
                    {
                        shape = result.shapeList.at(i);
                        for (int j = 0; j < shape.shapeResults.size(); ++j)
                        {
                            shapeCandidate = shape(j);
                            qDebug() << "shapeCandidate.shapeName is " << shapeCandidate.shapeName;
                        }
                    }
                    **/
                }
            }

            qDebug() << "Num results found:" << result.match.size();

            req.answerQueue->enqueue(result);

            qDebug() << "Responded to handwriting request.";
        }
        else
        {
            // Wait for the cycle time in stickies
            QThread::msleep(500); // JF increased this from 100 to 500 to cope with words
        }
    }
    qDebug() << "handwritingRecognition thread exiting";
}

bool handwritingRecognition::readyForRequests(void)
{
    return ready;
}

void handwritingRecognition::voShowResult( const QByteArray &resultsData)
{
    voShapeRecog->recognise( resultsData );
}

void handwritingRecognition::voStart(QVector<QVector<QPoint> > strokePoints)
{
    QObject::connect(&m_api, SIGNAL(resultReady(QByteArray)), this, SLOT(voShowResult(QByteArray)));

    m_api.getRecognitionResult(strokePoints);
}
