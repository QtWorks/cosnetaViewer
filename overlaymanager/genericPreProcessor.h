#ifndef GENERICPREPROCESSOR_H
#define GENERICPREPROCESSOR_H

#include "hwrTypes.h"

#include <QVector>
#include <QString>


class genericPreProcessor
{
public:
    genericPreProcessor();

    bool setSequenceFromString(QString seqString);
    bool applyFilterSequence(trace_t lines, trace_t &filteredLines);

private:
    bool applySmoothenTraceGroup(trace_t &lines);
    bool applyNormalizeSize(trace_t &lines);
    bool applyResampleTraceGroup(trace_t &lines, int newNumPoints);
    bool computeTraceLength(traceLine_t &line, int fromPoint, int toPoint, float& outLength);
    bool resampleTrace(traceLine_t &startTrace, int newNumPoints, traceLine_t &newTrace);
    bool getBoundingBox(trace_t &inTrace, float &xMin, float &yMin, float &xMax, float &yMax);
    bool affineTransform(trace_t &lines, float xScaleFactor,float yScaleFactor, float translateToX,float translateToY);

    QVector<preprocessIDs> sequence;
};

#endif // GENERICPREPROCESSOR_H
