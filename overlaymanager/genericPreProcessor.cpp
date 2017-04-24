#include "genericPreProcessor.h"

#include <QStringList>
#include <QDebug>
#ifdef Q_CC_GNU
#include <cmath>
#endif


preprocessIDs preProcID[3]  = { smoothenTraceGroup,  normalizeSize,  resampleTraceGroup};
QString preprocIDStrings[3] = {"smoothenTraceGroup","normalizeSize","resampleTraceGroup"};

genericPreProcessor::genericPreProcessor()
{
    sequence.clear();
}

bool genericPreProcessor::setSequenceFromString(QString seqString)
{
    qDebug() << "setSequenceFromString(" << seqString << ")";

    sequence.clear();

    if( seqString.startsWith("{") ) seqString.remove(0,1);
    if( seqString.endsWith("}") )   seqString.remove(seqString.length()-1,1);

    QStringList subStrings = seqString.split(",",QString::SkipEmptyParts);

    for(int s=0; s<subStrings.size(); s++)
    {
        if( subStrings[s].startsWith("CommonPreProc::") )
        {
            int idx;

            subStrings[s].remove(0,15);
            for(idx=0; idx<3; idx++)
            {
                if( subStrings[s] == preprocIDStrings[idx] )
                {
                    sequence.push_back(preProcID[idx]);
                    break;
                }
            }

            if( idx>=3 ) qDebug() << "Unrecognised filter:" << subStrings[s];
        }
        else
        {
            qDebug() << "Unrecognised untrimmed filter:" << subStrings[s];
        }
    }

    qDebug() << "Found" << sequence.size() << "filters.";

    return true;
}

bool genericPreProcessor::applyFilterSequence(trace_t lines, trace_t &filteredLines)
{
    filteredLines = lines;

    qDebug() << "applyFilterSequence()";

    for(int op=0; op<sequence.size(); op++)
    {
        switch(sequence[op])
        {
        case smoothenTraceGroup: applySmoothenTraceGroup(filteredLines);   break;
        case normalizeSize:      applyNormalizeSize(filteredLines);        break;
        case resampleTraceGroup: applyResampleTraceGroup(filteredLines, PREPROC_DEF_TRACE_DIMENSION);
                                                                                  break;
        default:                 qDebug() << "Error: unrecognised trace enum value";
        }
    }

    return true;
}

bool genericPreProcessor::getBoundingBox(trace_t &inTrace, float &xMin, float &yMin, float &xMax, float &yMax)
{
    xMin = inTrace.line[0][0].X;
    xMax = inTrace.line[0][0].X;
    yMin = inTrace.line[0][0].Y;
    yMax = inTrace.line[0][0].Y;

    for( int set=0; set<inTrace.line.size(); set++ )
    {
        for( int pt=0; pt<inTrace.line[set].size(); pt++ )
        {
            if( xMax < inTrace.line[set][pt].X ) xMax = inTrace.line[set][pt].X;
            if( xMin > inTrace.line[set][pt].X ) xMin = inTrace.line[set][pt].X;

            if( yMax < inTrace.line[set][pt].Y ) yMax = inTrace.line[set][pt].Y;
            if( yMin > inTrace.line[set][pt].Y ) yMin = inTrace.line[set][pt].Y;
        }
    }

    return true;
}

/**********************************************************************************
* AUTHOR		: Bharath A.
* DATE			: 03-SEP-2007
* NAME			: affineTransform
* DESCRIPTION	: scales the tracegroup according to the x and y scale factors.
*				  After scaling, the "referenceCorner" of the tracegroup is translated to
*				  (translateToX,translateToY)
* ARGUMENTS		: xScaleFactor - factor by which x dimension has to be scaled
*				  yScaleFactor - factor by which y dimension has to be scaled
*				  referenceCorner - corner to be retained after scaling and moved to (translateToX,translateToY)
* RETURNS		: SUCCESS on successful scale operation
* NOTES			:
* CHANGE HISTROY
* Author			Date				Description of change
*************************************************************************************/
bool genericPreProcessor::affineTransform(trace_t &lines, float xScaleFactor,float yScaleFactor,
                                     float translateToX,float translateToY)
{
    qDebug() << "Entered affineTransform: xScaleFactor=" << xScaleFactor << "yScaleFactor=" << yScaleFactor <<
                "translateToX" << translateToX << "translateToY" << translateToY;

    float xReference, yReference;
    float xMin=0.0f;
    float yMin=0.0f;
    float xMax=0.0f;
    float yMax=0.0f;

    int traceIndex;

    if(xScaleFactor <= 0)
    {
        qDebug() << "affineTransform Error: INVALID_X_SCALE_FACTOR";
        return false;
    }

    if(yScaleFactor <= 0)
    {
        qDebug() << "affineTransform Error: INVALID_Y_SCALE_FACTOR";
        return false;
    }

    if( ! getBoundingBox(lines,xMin,yMin,xMax,yMax) )
    {
        qDebug() << "Error: LTKTraceGroup::affineTransform()";
        return false;
    }
#if 0
    switch(referenceCorner)
    {

        case XMIN_YMIN:
#endif
            xReference=xMin;
            yReference=yMin;
#if 0
            break;

        case XMIN_YMAX:

            xReference=xMin;
            yReference=yMax;

            break;

        case XMAX_YMIN:

            xReference=xMax;
            yReference=yMin;

            break;

        case XMAX_YMAX:
            xReference=xMax;
            yReference=yMax;

            break;

        default: break;//define an exception

    }
#endif
    lines.XScaleFactor = 1.0; // Left in calc should we wish to re-instate the scale in the trace (set to 1 so we still work)
    lines.YScaleFactor = 1.0;

    int numTraces = lines.line.size();
    for(traceIndex=0; traceIndex < numTraces; ++traceIndex)
#if 1
    {
        for(int pt=0; pt<lines.line[traceIndex].size(); pt++)
        {
            lines.line[traceIndex][pt].X = ( (lines.line[traceIndex].at(pt).X * xScaleFactor)/lines.XScaleFactor) +
                                                   (translateToX - (xReference*(xScaleFactor/lines.XScaleFactor)) );
            lines.line[traceIndex][pt].Y = ( (lines.line[traceIndex].at(pt).Y * yScaleFactor)/lines.YScaleFactor) +
                                                   (translateToY - (yReference*(yScaleFactor/lines.YScaleFactor)));
        }

        qDebug() << "affineTransform: X[0]" << lines.line[traceIndex][0].X << "Y[0]" << lines.line[traceIndex][0].Y;
    }
#else
    {
        getTraceAt(traceIndex, trace);

        floatVector xVec;

        //no error handling required as the bounding box is found
        trace.getChannelValues(X_CHANNEL_NAME, xVec);


        floatVector yVec;


        trace.getChannelValues(Y_CHANNEL_NAME, yVec);


        numPoints = xVec.size();

        for(index=0; index < numPoints; index++)
        {
            //the additive term is to translate back the scaled tracegroup
            //so that the corner asked for is preserved at the destination
            //(translateToX,translateToY)
            x = ( (xVec.at(index) * xScaleFactor)/m_xScaleFactor) +
                 (translateToX - (xReference*(xScaleFactor/m_xScaleFactor)) );

            scaledXVec.push_back(x);

            //the additive term is to translate back the scaled tracegroup
            //so that the corner asked for is preserved at the destination
            //(translateToX,translateToY)
            y= ( (yVec.at(index) * yScaleFactor)/m_yScaleFactor) +
                 (translateToY - (yReference*(yScaleFactor/m_yScaleFactor)));

            scaledYVec.push_back(y);

        }

        trace.reassignChannelValues(X_CHANNEL_NAME,scaledXVec);

        trace.reassignChannelValues(Y_CHANNEL_NAME,scaledYVec);

        scaledXVec.clear();

        scaledYVec.clear();

        scaledTracesVec.push_back(trace);

    }

    m_traceVector = scaledTracesVec;

    m_xScaleFactor = xScaleFactor;
    m_yScaleFactor = yScaleFactor;

    LOG( LTKLogger::LTK_LOGLEVEL_DEBUG) <<
        "Enter: LTKTraceGroup::affineTransform()"<<endl;
#endif

    return true;
}


bool genericPreProcessor::applySmoothenTraceGroup(trace_t &lines)
{
    qDebug() << "applySmoothenTraceGroup()";

    int m_filterLength = PREPROC_DEF_SMOOTHFILTER_LENGTH;

    float sumX, sumY;          //partial sum
    QVector<float> newXChannel;
    QVector<float> newYChannel;

    int numTraces = lines.line.size();
    for(int traceIndex=0; traceIndex < numTraces; ++traceIndex)
    {
        newXChannel.clear();
        newYChannel.clear();

        qDebug() << "Smoothen group" << traceIndex;

        int numPoints = lines.line[traceIndex].size();

        for(int pointIndex = 0; pointIndex < numPoints ; ++pointIndex)
        {
            sumX = sumY = 0;

            for(int loopIndex = 0; loopIndex < m_filterLength; ++loopIndex)
            {
                int actualPtIndex = (pointIndex-loopIndex);

                //checking for bounding conditions
                if (actualPtIndex <0 )
                {
                    actualPtIndex = 0;
                }
                else if (actualPtIndex >= numPoints )
                {
                    actualPtIndex = numPoints -1;
                }

                //accumulating values
                sumX +=  lines.line[traceIndex][actualPtIndex].X;
                sumY +=  lines.line[traceIndex][actualPtIndex].Y;
            }

            //calculating average value
            sumX /= m_filterLength;
            sumY /= m_filterLength;

            // and add them to the new vector
            newXChannel.push_back(sumX);
            newYChannel.push_back(sumY);
        }

        // And populate the data
        for(int pointIndex = 0; pointIndex < numPoints ; ++pointIndex)
        {
            lines.line[traceIndex][pointIndex].X = newXChannel[pointIndex];
            lines.line[traceIndex][pointIndex].Y = newYChannel[pointIndex];
        }

        qDebug() << "smoothed X" << newXChannel;
        qDebug() << "smoothed Y" << newYChannel;

        newXChannel.clear();
        newYChannel.clear();
    }

    return true;
}

bool genericPreProcessor::applyNormalizeSize(trace_t &lines)
{
    float xScale;						//	scale factor along the x direction
    float yScale;						//	scale factor along the y direction
//    float aspectRatio;					//	aspect ratio of the trace group

//    QVector<trace_t> normalizedTracesVec;	// holds the normalized traces

    trace_t trace;							//	a trace of the trace group

    floatVector_t xVec;						//	x channel values of a trace
    floatVector_t yVec;						//	y channel values of a trace
    floatVector_t normalizedXVec;				//	normalized x channel values of a trace
    floatVector_t normalizedYVec;				//	normalized y channel values of a trace

    float scaleX, scaleY, offsetX, offsetY;
    float xMin,yMin,xMax,yMax;

    // getting the bounding box information of the input trace group

    if( ! getBoundingBox(lines, xMin,yMin,xMax,yMax) )
    {
        qDebug() << "applyNormalizeSize(): getBoundingBox Error: normalizeSize";
        return false;
    }

    qDebug() << "applyNormalizeSize(): min=("<<xMin<<yMin<<") max=("<<xMax<<yMax<<")";

    // Copy output from input
//    filteredLines = lines;
#if 0
    //	width of the bounding box at scalefactor = 1

    xScale = ((float)fabs(xMax - xMin))/inTraceGroup.getXScaleFactor();
    qDebug() << "xScale = " <<  xScale;

    // height of the bounding box at scalefactor = 1

    yScale = ((float)fabs(yMax - yMin))/inTraceGroup.getYScaleFactor();
    qDebug() << "yScale = " <<  yScale;

    if(m_preserveAspectRatio)
    {
        if(yScale > xScale)
        {
            aspectRatio = (xScale > EPS) ? (yScale/xScale) : m_aspectRatioThreshold + EPS;
        }
        else
        {
            aspectRatio = (yScale > EPS) ? (xScale/yScale) : m_aspectRatioThreshold + EPS;
        }
        if(aspectRatio > m_aspectRatioThreshold)
        {
            if(yScale > xScale)
                xScale = yScale;
            else
                yScale = xScale;
        }
    }
#endif
    offsetY = 0.0f;
    if( PREPROC_DEF_PRESERVE_RELATIVE_Y_POSITION )
    {
        offsetY = (yMin+yMax)/2.0f;
    }
#if 0
    if(xScale <= (PREPROC_DEF_DOT_THRESHOLD * DEFAULT_X_DPI) && yScale <= (PREPROC_DEF_DOT_THRESHOLD * DEFAULT_Y_DPI))
    {
        offsetX = PREPROC_DEF_NORMALIZEDSIZE/2;
        offsetY += PREPROC_DEF_NORMALIZEDSIZE/2;

        outTraceGroup.emptyAllTraces();

        for(int traceIndex=0;traceIndex<inTraceGroup.getNumTraces();++traceIndex)
        {
            trace_t tempTrace;

            inTraceGroup.getTraceAt(traceIndex,tempTrace);

            QVector<float> newXChannel(tempTrace.getNumberOfPoints(),offsetX);
            QVector<float> newYChannel(tempTrace.getNumberOfPoints(),offsetY);

            tempTrace.reassignChannelValues(X_CHANNEL_NAME, newXChannel);
            tempTrace.reassignChannelValues(Y_CHANNEL_NAME, newYChannel);

            outTraceGroup.addTrace(tempTrace);
        }

        return true;
    }
#else
    xScale = ((float)fabs(xMax - xMin));
    qDebug() << "xScale = " <<  xScale;

    // height of the bounding box at scalefactor = 1

    yScale = ((float)fabs(yMax - yMin));
    qDebug() << "yScale = " <<  yScale;

#endif
    // finding the final scale and offset values for the x channel
    if((!PREPROC_DEF_PRESERVE_ASPECT_RATIO )&&(xScale < (PREPROC_DEF_SIZE_THRESHOLD*DEFAULT_X_DPI)))
    {
        scaleX = 1.0f;
        offsetX = PREPROC_DEF_NORMALIZEDSIZE/2.0 ;
    }
    else
    {
        scaleX =  PREPROC_DEF_NORMALIZEDSIZE / xScale ;
        offsetX = 0.0;
    }

    // finding the final scale and offset values for the y channel

    if((!PREPROC_DEF_PRESERVE_ASPECT_RATIO )&&(yScale < (PREPROC_DEF_SIZE_THRESHOLD*DEFAULT_Y_DPI)))
    {
        offsetY += PREPROC_DEF_NORMALIZEDSIZE/2;
        scaleY = 1.0f;
    }
    else
    {
        scaleY = PREPROC_DEF_NORMALIZEDSIZE / yScale ;
    }

    //scaling the copy of the inTraceGroup in outTraceGroup according to new scale factors
    //and translating xmin_ymin of the trace group bounding box to the point (offsetX,offsetY).
    //Even though absolute location has to be specified for translateToX and translateToY,
    //since offsetX and offsetY are computed with respect to origin, they serve as absolute values

    if( ! affineTransform(lines,scaleX,scaleY,offsetX,offsetY) )
    {
        qDebug() << "applyNormalizeSize: affineTransform failed";
        return false;
    }

    return true;
}

bool genericPreProcessor::computeTraceLength(traceLine_t &line, int fromPoint, int toPoint, float& outLength)
{
    qDebug() << "Entered LTKPreprocessor::computeTraceLength";

    int NoOfPoints = line.size();

    if(  (fromPoint < 0 || fromPoint > (NoOfPoints-1))
        || (toPoint < 0 || toPoint > (NoOfPoints-1))  )
    {
        qDebug() <<"computeTraceLength Error : index out of bounds";
        return false;
    }

    float xDiff, yDiff;
    float pointDistance;

    outLength = 0;

    for(int pointIndex = fromPoint; pointIndex < toPoint; ++pointIndex)
    {
        xDiff = line[pointIndex].X - line[pointIndex+1].X;
        yDiff = line[pointIndex].Y - line[pointIndex+1].Y;

        pointDistance = (float) (sqrt(xDiff*xDiff + yDiff*yDiff)); //distance between 2 points.

        outLength += pointDistance; // finding the length of trace.

        qDebug() << "point distance: " <<  pointDistance;
    }

    return true;
}

bool genericPreProcessor::resampleTrace(traceLine_t &startTrace,int newNumPoints,traceLine_t &newTrace)
{
    // point based (not interpoint dist based)

    int numTracePoints = startTrace.size();

    if( numTracePoints==0 )
    {
        qDebug() << "resampleTrace: no points in trace to be resampled!";
        return false;
    }
    else if( numTracePoints == 1 )
    {
        qDebug() << "resampleTrace: resampling a single point to" << newNumPoints;

        newTrace.clear();

        for(int n=0; n<newNumPoints; n++)
        {
            newTrace.push_back(startTrace.at(0));
        }

        return true;
    }

    newTrace.clear();

    qDebug() << "resampleTrace to" << newNumPoints << "points from" << numTracePoints << "points";

    QVector<float> distanceVec;
    float          xDiff, yDiff;
    float          pointDistance;
    float          unitLength = 0.0;

    for( int pointIndex = 0; pointIndex < (numTracePoints-1); ++pointIndex)
    {
        xDiff = startTrace.at(pointIndex).X - startTrace.at(pointIndex+1).X;
        yDiff = startTrace.at(pointIndex).Y - startTrace.at(pointIndex+1).Y;

//        qDebug() << "xDiff" << xDiff << "yDiff" << yDiff;

        pointDistance = (float) (sqrt(xDiff*xDiff + yDiff*yDiff)); //distance between 2 points.

        unitLength += pointDistance; // finding the length of trace.

        distanceVec.push_back(pointDistance); //storing distances between every 2 consecutive points.
    }

    qDebug() << "applyResampleTrace: unitLength" << unitLength << "point distances =" << distanceVec
             << "newNumPoints=" << newNumPoints;

    unitLength /= (newNumPoints -1.0);

    // Build up the output point at a time
    tracePoint_t sample;

    // Starting sample
    sample.X = startTrace[0].X;
    sample.Y = startTrace[0].Y;

    newTrace.push_back(sample);

    float  balanceDistance = 0.0;   // Distance between the last resampled point and the next raw sample point
    float  measuredDistance;

    float  m1, m2;
    float  xTemp, yTemp;

    int    ptIndex = 0;
    int    currentPointIndex =0;				//	index of the current point

    for(int pointIndex = 1; pointIndex < (newNumPoints-1); ++pointIndex)
    {
        measuredDistance = balanceDistance;

        while(measuredDistance < unitLength)
        {
            measuredDistance += distanceVec.at(ptIndex++);

            if(ptIndex == 1)
            {
                currentPointIndex = 1;
            }
            else
            {
                currentPointIndex++;
            }
        }

        if(ptIndex < 1) ptIndex = 1;

        m2 = measuredDistance - unitLength;
        m1 = distanceVec.at(ptIndex -1) - m2;

        if( fabs(m1+m2) > EPS)
        {
            xTemp =  (m1* startTrace.at(currentPointIndex).X + m2* startTrace.at(currentPointIndex -1).X)/(m1+m2);
            yTemp =  (m1* startTrace.at(currentPointIndex).Y + m2* startTrace.at(currentPointIndex -1).Y)/(m1+m2);
        }
        else
        {
            xTemp = startTrace.at(currentPointIndex).X;
            yTemp = startTrace.at(currentPointIndex).Y;
        }

        sample.X = xTemp;
        sample.Y = yTemp;

//        qDebug() << "Current point #" << currentPointIndex << "resampled to:" << xTemp << yTemp;

        newTrace.push_back(sample);

        balanceDistance = m2;
    }

    // End point
    sample.X = startTrace[startTrace.size()-1].X;
    sample.Y = startTrace[startTrace.size()-1].Y;

    newTrace.push_back(sample);

    qDebug() << "resampled trace of" << startTrace.size() << "points to" << newTrace.size();

    return true;
}


bool genericPreProcessor::applyResampleTraceGroup(trace_t &lines, int newNumPoints)
{
    qDebug() << "applyResampleTraceGroup()";

    int numTracePoints = 0;
    for(int t=0; t<lines.line.size(); t++)
    {
        numTracePoints += lines.line[t].size();
    }

    if(numTracePoints==0)
    {
        qDebug() << "No trace points to resample.";
        return false;
    }

    qDebug() << "applyResampleTraceGroup - currently have" << numTracePoints << "points";

    QVector<int> pointsPerTrace(lines.line.size(),0);

    // Point based (as per nn.cfg)
    int total = 0;

    for(int t=0; t<lines.line.size(); t++)
    {
        if( t == (lines.line.size() - 1))
            pointsPerTrace[t] = ((newNumPoints - lines.line.size()) - total);
        else
        {
            pointsPerTrace[t] = int(floor((newNumPoints - lines.line.size()) * lines.line[t].size() /
                                                                                    float(numTracePoints) ));
        }

        total += pointsPerTrace[t];
    }

    qDebug() << "Points per trace:" << pointsPerTrace;

    // Build up a new set of lines
    trace_t filteredLines;

    traceLine_t  newLine;

    // And resize each trace appropriately
    for(int i=0; i < lines.line.size(); i++)
    {
        qDebug() << "resize trace group" << i << "to" << pointsPerTrace[i]+1 << "points.";
        if( ! resampleTrace(lines.line[i], pointsPerTrace[i]+1, newLine ) )
        {
            qDebug() << "applyResampleTraceGroup: failed to resize trace group" << i;
            return false;
        }

        filteredLines.line.push_back(newLine);
    }

    lines = filteredLines;
#if 1
    int finalTotal = 0;
    for(int t=0; t<lines.line.size(); t++)
    {
        finalTotal += lines.line[t].size();
    }
    qDebug() << "applyResampleTraceGroup: numTracePoints after resize" << finalTotal;
#endif

    return true;
}
