#include <QString>
#include <QFile>
#include <QDir>
#include <QStringList>
#include <QApplication>
#ifdef Q_CC_GNU
#include <cmath>
#endif

#include <QDebug>

#include "NNRecogniser.h"

NNRecogniser::NNRecogniser()
{
    // Our config
    m_dtwEuclideanFilter = 15;
    m_nearestNeighbors   = 4;
}



bool NNRecogniser::tokeniseString(const QString&    inputString,
                                  QVector<QString>& outTokens)
{
    qDebug() << "tokeniseString";

    //	clearing the token list
    int inputStrLength = inputString.length();

    outTokens.clear();

    QStringList words = inputString.split(QRegExp("["TOKENIZE_DELIMITER"]"),QString::SkipEmptyParts);

    for( int t=0; t<words.size(); t++ )
    {
        outTokens.append(words[t]);
    }

    return true;
}


bool NNRecogniser::readHeader(QFile &inFile, QMap<QString,QString> &header)
{
    // Read the first 50 characters, and look for the header length
    char headerStart[51];

    if( inFile.read(headerStart,50) != 50 )
    {
        qDebug() << "Failed to read MDT header start";
        return false;
    }

    // Extract header length
    char *ptr = strstr(headerStart, HEADERLEN);
    if(ptr == NULL)
    {
        qDebug() << "Failed to find MDT header length";
        return false;
    }

    strtok(ptr, "=");

    char *headerLenPtr = strtok( NULL , ">" );

    if(headerLenPtr == NULL)
    {
        qDebug() << "Failed to find = in MDT header length";
        return false;
    }

    int headerLen = atoi(headerLenPtr);
    if( headerLen < 10 )
    {
        qDebug() << "Bad MDT header length: " << headerLen;
        return false;
    }

    qDebug() << "Header Length:" << headerLen;

    // Now read the full header from the start again
    inFile.seek(0);
    char *headerData = new char [headerLen+1];

    if( inFile.read(headerData, headerLen) != headerLen )
    {
        qDebug() << "Failed to read full header length:" << headerLen;
        return false;
    }

    // And get the tokens
    QVector<QString> strTokens;
    tokeniseString(headerData, strTokens);

    int strTokensSize = strTokens.size();

    for(int indx=0; indx+1 <strTokensSize ;indx=indx+2)
    {
        header[strTokens.at(indx)] = strTokens.at(indx+1);
    }

    return true;
}

bool NNRecogniser::validateDataParams(void)
{
    bool ok;

    // Based on the short-cuts that I have taken so far
    if( dataParams[SIZEOFFLOAT].toInt(&ok) != 4 )
    {
        qDebug() << "Unsupported float size:" << dataParams[SIZEOFFLOAT] << "ConvSuccess=" << ok;
        return false;
    }
    if( dataParams[SIZEOFUINT].toInt(&ok)  != 4 )
    {
        qDebug() << "Unsupported UINT size" << dataParams[SIZEOFFLOAT] << "ConvSuccess=" << ok;
        return false;
    }
    if( dataParams[SIZEOFINT].toInt(&ok)   != 4 )
    {
        qDebug() << "Unsupported INT size" << dataParams[SIZEOFFLOAT] << "ConvSuccess=" << ok;
        return false;
    }

    if( dataParams[BYTEORDER] != QString("LE") )
    {
        qDebug() << "Byteorder != LE. Byteorder=" << dataParams[BYTEORDER];
        return false;
    }
    // Need a check for Arm endianess (i.e. this code is running on Arm)

    return true;
}


static bool createInPointFloatShapeFeature(PointFloatShapeFeature_t * &shapeFeature,
                                           QVector<float>             &floatFeatureVector)
{
    if(floatFeatureVector.size() != 5)
    {
        qDebug() << "createInPointFloatShapeFeature: bad number of features (not 5):" << floatFeatureVector.size();
        return false;
    }

    shapeFeature = new PointFloatShapeFeature_t;

    if( ! shapeFeature )
    {
        qDebug() << "createInPointFloatShapeFeature: alloc failure";
        return false;
    }

    shapeFeature->X        = floatFeatureVector[0];
    shapeFeature->Y        = floatFeatureVector[1];
    shapeFeature->sinTheta = floatFeatureVector[2];
    shapeFeature->cosTheta = floatFeatureVector[3];

    if( floatFeatureVector[4] == 0.0 )
    {
        shapeFeature->penUp = false;
    }
    else if( floatFeatureVector[4] == 1.0 )
    {
        shapeFeature->penUp = true;
    }
    else
    {
        // Do a munge for now
        if( floatFeatureVector[4]<0.5 ) shapeFeature->penUp = false;
        else                            shapeFeature->penUp = true;

        qDebug() << "Munge penUp value to " << shapeFeature->penUp << "from" << floatFeatureVector[4];
    }

    return true;
}

bool NNRecogniser::pointFloatShapeExtractFeatures(trace_t &traceGroup, PointFloatShape_t &outTrace)
{
    if( traceGroup.line.size() < 1 )
    {
        qDebug() << "pointFloatShapeExtractFeatures() - no input strokes";
        return false;
    }

    // The shape feature is just all the points concatenated together
    outTrace.clear();

    for(int stroke=0; stroke<traceGroup.line.size(); stroke++)
    {
        for(int pt=0; pt<traceGroup.line[stroke].size(); pt++)
        {
            PointFloatShapeFeature_t sample;

            sample.X     = traceGroup.line[stroke][pt].X;
            sample.Y     = traceGroup.line[stroke][pt].Y;
            sample.penUp = false;

            outTrace.push_back(sample);
        }
    }

    QVector<float> delta_x;
    QVector<float> delta_y;

    for(int pt=0; pt<outTrace.size()-1; pt++)
    {
        delta_x.push_back( outTrace[pt+1].X - outTrace[pt].X );
        delta_y.push_back( outTrace[pt+1].Y - outTrace[pt].Y );
    }

    float sqsum = sqrt( pow(outTrace[0].X,2)+ pow(outTrace[0].Y,2))+ EPS;

    outTrace[0].sinTheta = (1+outTrace[0].Y/sqsum)*PREPROC_DEF_NORMALIZEDSIZE/2;
    outTrace[0].cosTheta = (1+outTrace[0].X/sqsum)*PREPROC_DEF_NORMALIZEDSIZE/2;

    for(int pt=1; pt<outTrace.size(); pt++ )
    {
        sqsum = sqrt(pow(delta_x[pt-1],2) + pow(delta_y[pt-1],2))+EPS;

        outTrace[pt].sinTheta = (1+delta_y[pt-1]/sqsum)*PREPROC_DEF_NORMALIZEDSIZE/2;
        outTrace[pt].cosTheta = (1+delta_x[pt-1]/sqsum)*PREPROC_DEF_NORMALIZEDSIZE/2;
    }

    return true;
}


// NB We assume that the MDT file is in the same directory as the executable.
bool NNRecogniser::loadModelData(QString fileName)
{
    QFile inFile(QApplication::applicationDirPath() + QDir::separator() + fileName);

    qDebug() << "Entering NNShapeRecognizer::loadModelData()";

    qint32 numofShapes = 0;

    // variable for shape Id
    qint32 classID    = -1;

    //Algorithm version
    QString algoVersionReadFromMDT = "";

    // NB This is a binary read
    if( ! inFile.open(QIODevice::ReadOnly) )
    {
        qDebug() << "Failed to open MDT file:" << fileName;

        // Failed to start playing the saved discussion.
        return false;
    }

    // Read the header and parse into a QMap: dataParams[QString:index]=QString:value
    if( ! readHeader(inFile, dataParams) )
    {
        inFile.close();

        qDebug() << "Failed to read header from MDT file:"
                 << (QApplication::applicationDirPath() + QDir::separator() + fileName);
        return false;
    }

//    qDebug() << "Header:" << dataParams;

    if( ! validateDataParams() )
    {
        inFile.close();

        qDebug() << "Parameter validation failed:" << dataParams;
        return false;
    }

    // Initialise the pre-processor
    preprocess.setSequenceFromString(dataParams[PREPROC_SEQ]);

    // Assume binary mode from here on

    QString featureExtractor = dataParams[FE_NAME];
    int     floatSize        = dataParams[SIZEOFFLOAT].toInt();
    int     intSize          = dataParams[SIZEOFINT].toInt();

    classFromIndex.clear();

    NNShapeSample_t shapeSampleFeatures;
    QVector<PointFloatShapeFeature_t *> shapeFeatureVector;

    // Read the data
    inFile.read((char*) &numofShapes, dataParams[SIZEOFSHORTINT].toInt());

    while( ! inFile.atEnd() )
    {
        if( intSize != inFile.read((char *)&classID, intSize) )
        {
            qDebug() << "ClassId read failed (prolly foef())";
            break;
        }

        int numberOfFeatures;
        int featureDimension;

        inFile.read((char*) &numberOfFeatures, intSize);
        inFile.read((char*) &featureDimension, intSize);

        int featureIndex = 0;

        PointFloatShapeFeature_t            *shapeFeature;
        shapeFeatureVector.clear();

        for ( ; featureIndex < numberOfFeatures ; featureIndex++)
        {
            QVector<float> floatFeatureVector;
            int            featureValueIndex = 0;

            for(; featureValueIndex < featureDimension ; featureValueIndex++)
            {
                float featureValue = 0.0f;

                if( floatSize != inFile.read((char*) &featureValue, floatSize) )
                {
                    break;
                }

                floatFeatureVector.push_back(featureValue);
            }

            // NB This allocates the structure too.
            if( ! createInPointFloatShapeFeature(shapeFeature,floatFeatureVector) )
            {
                qDebug() <<"Error: Number of features extracted from a trace is not correct";
                return false;
            }

            shapeFeatureVector.push_back(shapeFeature);
        }

//        qDebug() << "Feature class" << classID << "Num features:" << shapeFeatureVector.size() <<
//                    "numberOfFeatures" << numberOfFeatures;

        //Set the feature vector and class id to the shape sample features
        shapeSampleFeatures.classID = classID;
        shapeSampleFeatures.samples = shapeFeatureVector;

        //Adding all shape sample feature to the prototypeset
        m_prototypeSet.push_back(shapeSampleFeatures);
#if 0
        //Add to Map
        if(	m_shapeIDNumPrototypesMap.find(classID)==m_shapeIDNumPrototypesMap.end())
        {
            m_shapeIDNumPrototypesMap[classID] = 1;
        }
        else
        {
            ++(m_shapeIDNumPrototypesMap[classID]);
        }
#else
        // Allow quick lookups of class from the shape index
        classFromIndex[m_prototypeSet.size()-1] = classID;
#endif
        classID = -1;
    }

    // And finish up.
    inFile.close();

    qDebug() << "Exiting NNShapeRecognizer::loadModelData() - loaded " << m_prototypeSet.size() << "features.";

    return true;
}


float NNRecogniser::getDistance(PointFloatShapeFeature_t *a, PointFloatShapeFeature_t *b)
{
    // PointFloatShapeFeature:
    float xDiff = 0, yDiff = 0, sinthetaDiff=0, costhetaDiff=0;

    xDiff = a->X - b->X;
    yDiff = a->Y - b->Y;

    sinthetaDiff = a->sinTheta - b->sinTheta;
    costhetaDiff = a->cosTheta - b->cosTheta;

    return ( (xDiff * xDiff) + (yDiff * yDiff) +
             (sinthetaDiff * sinthetaDiff) + (costhetaDiff * costhetaDiff) );
}


bool NNRecogniser::computeEuclideanDistance(int protoNum, PointFloatShape_t features, float &euclideanDistance)
{
    if(m_prototypeSet[protoNum].samples.size() != features.size() )
    {
        qDebug() << "computeEuclideanDistance comparing a set of" << m_prototypeSet[protoNum].samples.size() <<
                    "points with a set of" << features.size() << "points FAILS!";
        return false;
    }

    for(int pt=0; pt<m_prototypeSet[protoNum].samples.size(); pt++)
    {
        euclideanDistance += getDistance( m_prototypeSet[protoNum].samples[pt], &(features[pt]));
    }

    return true;
}


#if 0
bool NNShapeRecognizer::computeConfidence()
{
    qDebug() << "Entering NNShapeRecognizer::computeConfidence()";

    /******************************************************************/
    /*******************VALIDATING INPUT ARGUMENTS*********************/
    /******************************************************************/
    if(m_neighborInfoVec.empty())
    {
        LOG(LTKLogger::LTK_LOGLEVEL_ERR) << "Error: "<< ENEIGHBOR_INFO_VECTOR_EMPTY << " " <<
            getErrorMessage(ENEIGHBOR_INFO_VECTOR_EMPTY) <<
            " NNShapeRecognizer::computeConfidence()" << endl;

        LTKReturnError(ENEIGHBOR_INFO_VECTOR_EMPTY);
    }
    // Temporary vector to store the recognition results
    LTKShapeRecoResult outResult;
    vector<pair<int,float> > classIdSimilarityPairVec;
    pair<int, float> classIdSimilarityPair;

    // Temporary vector to store the distinct classes appearing in distIndexPairVector
    intVector distinctClassVector;
    intVector::iterator distinctClassVectorIter;

    vector <LTKShapeRecoResult>::iterator resultVectorIter = m_vecRecoResult.begin();
    vector <LTKShapeRecoResult>::iterator resultVectorIterEnd = m_vecRecoResult.end();

    // Variable to store sum of distances to all the prototypes in distIndexPairVector
    float similaritySum = 0.0f;
    // Temporary variable to store the confidence value.
    float confidence = 0.0f;

    // Confidence computation for the NN (1-NN) Classifier
    if(m_nearestNeighbors == 1)
    {
        vector <struct NeighborInfo>::iterator distIndexPairIter = m_neighborInfoVec.begin();
        vector <struct NeighborInfo>::iterator distIndexPairIterEnd = m_neighborInfoVec.end();


        for(; distIndexPairIter != distIndexPairIterEnd; ++distIndexPairIter)
        {
            //Check if the class is already present in distinctClassVector
            //The complexity of STL's find() is linear, with atmost last-first comparisons for equality
            distinctClassVectorIter = find(distinctClassVector.begin(), distinctClassVector.end(), (*distIndexPairIter).classId);

            //The distinctClassVectorIter will point to distinctClassVector.end() if the class is not present in distinctClassVector
            if(distinctClassVectorIter == distinctClassVector.end())
            {
                //outResult.setShapeId( (*distIndexPairIter).classId );
                classIdSimilarityPair.first = (*distIndexPairIter).classId ;
                float similarityValue = SIMILARITY((*distIndexPairIter).distance);
                //outResult.setConfidence(similarityValue);
                classIdSimilarityPair.second = similarityValue;
                similaritySum += similarityValue;
                //m_vecRecoResult.push_back(outResult);
                classIdSimilarityPairVec.push_back(classIdSimilarityPair);
                distinctClassVector.push_back((*distIndexPairIter).classId);
            }
        }

        int classIdSimilarityPairVecSize = classIdSimilarityPairVec.size();
        for( int i = 0 ; i < classIdSimilarityPairVecSize ; ++i)
        {
            int classID = classIdSimilarityPairVec[i].first;
            confidence = classIdSimilarityPairVec[i].second;
            confidence /= similaritySum;
            outResult.setConfidence(confidence);
            outResult.setShapeId(classID);
            if(confidence > 0)
            m_vecRecoResult.push_back(outResult);

        }
        classIdSimilarityPairVec.clear();

    }
    // Computing confidence for k-NN classifier, implementation of the confidence measure described in paper (cite)
    else
    {
        if(m_nearestNeighbors >= m_neighborInfoVec.size())
        {
            LOG(LTKLogger::LTK_LOGLEVEL_DEBUG) <<
                "m_nearestNeighbors >= m_prototypeSet.size(), using distIndexPairVector.size() for m_nearestNeighbors instead" << endl;
            m_nearestNeighbors = m_neighborInfoVec.size();
        }
//        vector<pair<int,float> > classIdSimilarityPairVec;
  //      pair<int, float> classIdSimilarityPair;

        // Variable to store the maximum of the number of samples per class.
        int maxClassSize = (max_element(m_shapeIDNumPrototypesMap.begin(), m_shapeIDNumPrototypesMap.end(), &compareMap))->second;
        LOG(LTKLogger::LTK_LOGLEVEL_DEBUG) << "maxClassSize: " <<maxClassSize<< endl;

        // Vector to store the cumulative similarity values
        vector<float> cumulativeSimilaritySum;

        // Populate the values in cumulativeSimilaritySum vector for the top m_nearestNeighbors prototypes
        // Assumption is m_nearestNeighbors >= MIN_NEARESTNEIGHBORS and
        // m_nearestNeighbors < dtwEuclideanFilter, validation done in recognize()

        LOG(LTKLogger::LTK_LOGLEVEL_DEBUG) << "Displaying cumulativeSimilaritySum..." << endl;
        int i = 0;
        for( i = 0 ; i < m_nearestNeighbors ; ++i)
        {
            //outResult.setShapeId(m_neighborInfoVec[i].classId);
            classIdSimilarityPair.first = m_neighborInfoVec[i].classId;
            float similarityValue = SIMILARITY((m_neighborInfoVec[i]).distance);
//            outResult.setConfidence(similarityValue);
            classIdSimilarityPair.second = similarityValue;
//            classIdSimilarityPairVector.push_back(outResult);
            classIdSimilarityPairVec.push_back(classIdSimilarityPair);
            similaritySum += similarityValue;
            cumulativeSimilaritySum.push_back(similaritySum);

            // Logging the cumulative similarity values for debugging
            LOG(LTKLogger::LTK_LOGLEVEL_DEBUG) << "classID:" <<
                m_neighborInfoVec[i].classId << " confidence:" <<
                similarityValue << endl;

            LOG(LTKLogger::LTK_LOGLEVEL_DEBUG) << i << ": " << similaritySum << endl;
        }

//        for(i = 0 ; i < classIdSimilarityPairVector.size() ; ++i)
        for(i = 0 ; i < classIdSimilarityPairVec.size() ; ++i)
        {
//            int classID = classIdSimilarityPairVector[i].getShapeId();
            int classID = classIdSimilarityPairVec[i].first;

            int finalNearestNeighbors = 0;

            //Check if the class is already present in distinctClassVector
            distinctClassVectorIter = find(distinctClassVector.begin(), distinctClassVector.end(), classID);

            //The distinctClassVectorIter will point to distinctClassVector.end() if the class is not present in distinctClassVector
            if(distinctClassVectorIter == distinctClassVector.end())
            {
                distinctClassVector.push_back(classID);
                confidence = 0.0f;

                //If the confidence is based on Adaptive k-NN scheme,
                //Computing number of nearest neighbours for the class to be used for computation of confidence
                if(m_adaptivekNN == true )
                {

                    int sizeProportion = (int)ceil(1.0*m_nearestNeighbors*m_shapeIDNumPrototypesMap[classID]/maxClassSize);
                    LOG(LTKLogger::LTK_LOGLEVEL_DEBUG)<<"sizeProportion of class " <<classID<<" is "<<sizeProportion<<endl;

                    // Computing min(sizeProportion, m_shapeIDNumPrototypesMap[classID])
                    int upperBound = (sizeProportion < m_shapeIDNumPrototypesMap[classID]) ? sizeProportion:m_shapeIDNumPrototypesMap[classID];
                    LOG(LTKLogger::LTK_LOGLEVEL_DEBUG)<<"upperBound: " <<upperBound<<endl;

                    // Computing max(upperBound, MIN_NEARESTNEIGHBORS)
                    finalNearestNeighbors = (MIN_NEARESTNEIGHBORS > upperBound) ? MIN_NEARESTNEIGHBORS:upperBound;
                }
                //Else, compute kNN based confidence
                else if(m_adaptivekNN == false)
                {
                    finalNearestNeighbors = m_nearestNeighbors;
                }
                else
                {
                    LOG(LTKLogger::LTK_LOGLEVEL_ERR) <<"Error: " << ECONFIG_FILE_RANGE << " " <<
            "m_adaptivekNN should be true or false" <<
            " NNShapeRecognizer::computeConfidence()" << endl;
                    LTKReturnError(ECONFIG_FILE_RANGE);
                }


                LOG(LTKLogger::LTK_LOGLEVEL_DEBUG)<<"finalNearestNeighbors: " <<finalNearestNeighbors<<endl;

                for( int j = 0 ; j < finalNearestNeighbors ; ++j)
                {
                    if(classID == classIdSimilarityPairVec[j].first)
                    {
                        confidence += classIdSimilarityPairVec[j].second;
                    }
                }
                confidence /= cumulativeSimilaritySum[finalNearestNeighbors-1];

                outResult.setShapeId(classID);
                outResult.setConfidence(confidence);

                if(confidence > 0)
                {
                    LOG(LTKLogger::LTK_LOGLEVEL_DEBUG)<<"classId: " <<classID<<" confidence: "<<confidence<<endl;

                    m_vecRecoResult.push_back(outResult);
                }
            }
        }
        classIdSimilarityPairVec.clear();
    }

    //Sort the result vector in descending order of confidence
    sort(m_vecRecoResult.begin(), m_vecRecoResult.end(), sortResultByConfidence);

    //Logging the results at info level
    LOG(LTKLogger::LTK_LOGLEVEL_DEBUG)<<"Printing the Classes and respective Confidences" <<endl;

    for( int i=0; i < m_vecRecoResult.size() ; i++)
    {
        LOG(LTKLogger::LTK_LOGLEVEL_DEBUG)<<"Class ID: " <<m_vecRecoResult[i].getShapeId()
            <<" Confidence: "<<m_vecRecoResult[i].getConfidence()
            <<endl;
    }

    distinctClassVector.clear();

    LOG(LTKLogger::LTK_LOGLEVEL_DEBUG) << "Exiting " <<
        "NNShapeRecognizer::computeConfidence()" << endl;

    return true;
}
#endif




bool NeighborInfoLessThan(struct NeighborInfo &a, struct NeighborInfo &b)
{
    return a.distance < b.distance;
}


bool NNRecogniser::recognise(trace_t lines, shapeRecoResultSet_t &results)
{
    qDebug() << "recognise" << lines.line.size() << "lines";

    // TODO: check for empty traces

    // Filter lines before comparing
    trace_t preprocessedLines;

    if( ! preprocess.applyFilterSequence(lines,preprocessedLines) )
    {
        qDebug() << "Failed to preprocess input lines.";
        return false;
    }

    // Extract features from input
    PointFloatShape_t features;

    if( ! pointFloatShapeExtractFeatures(preprocessedLines, features) )
    {
        qDebug() << "Failed to extract features.";
        return false;
    }

    //
    int numPrototypes      = m_prototypeSet.size();
    int dtwEuclideanFilter = (m_dtwEuclideanFilter == EUCLIDEAN_FILTER_OFF) ?
                                     EUCLIDEAN_FILTER_OFF : (int)((m_dtwEuclideanFilter * numPrototypes) / 100);
    if(dtwEuclideanFilter == 0)  dtwEuclideanFilter = EUCLIDEAN_FILTER_OFF;

    if( dtwEuclideanFilter != EUCLIDEAN_FILTER_OFF && dtwEuclideanFilter < m_nearestNeighbors)
    {
        qDebug() << "Euclidean filter is out of range";
        return false;
    }

    // Config/input values fixed here
    int   numChoices = 5;       // Num options returned. Normally an input, but use 5 here
//    float confThreshold = 0.0;  // Valid: [0:1]


    //Variable to store the DTW distance.
    float dtwDistance = 0.0f;

    //Variable to store the Euclidean distance.
    float euclideanDistance = 0.0f;

    //Temporary variable to be used to populate distIndexPairVector
    struct NeighborInfo tempPair;

    // NB We assume DTW distance here as our dataset uses that

    QVector<struct NeighborInfo> eucDistIndexPairVector;

    // Loop through all the classID entries (basically, we don't do the sunset thing here)
    for(int proto=0; proto<numPrototypes; proto++)
    {
        euclideanDistance = 0.0;

        if( ! computeEuclideanDistance(proto,features,euclideanDistance) )
        {
            qDebug() << "NNShapeRecognizer::recognize() - failed to compute euclidean distance";
            return false;
        }

        tempPair.distance          = euclideanDistance;
        tempPair.classId           = classFromIndex[proto];
        tempPair.prototypeSetIndex = proto;

        eucDistIndexPairVector.push_back(tempPair);
    }

    // Sort the results by the difference
    qSort(eucDistIndexPairVector.begin(),eucDistIndexPairVector.end(),NeighborInfoLessThan);

    // Take top m_dtwEuclideanFilter (15) best, and then calc the DTW distance for these
    eucDistIndexPairVector.remove(m_dtwEuclideanFilter,eucDistIndexPairVector.size()-m_dtwEuclideanFilter);

    // Calculate the DTW distance for the top 15
 // TODO: Put this back in.
#if 0
    for(int pair=0; pair<eucDistIndexPairVector.size(); pair++)
    {
        if( ! computeDTWDistance(eucDistIndexPairVector[pair].prototypeSetIndex,
                                 features,eucDistIndexPairVector[pair].distance) )
        {
            qDebug() << "NNShapeRecognizer::recognize() - failed to compute DTW distance";
            return false;
        }
    }
#endif
    // Sort the results by the difference
    qSort(eucDistIndexPairVector.begin(),eucDistIndexPairVector.end(),NeighborInfoLessThan);

    // Later? Reject the sample if the similarity of the nearest sample is lower than m_rejectThreshold specified by the user.
#if 0
    //Compute the confidences of the classes appearing in distIndexPairVector
    //outResultVector is an output argument, populated in computeConfidence()
    if((errorCode = computeConfidence()) != SUCCESS)
    {
        LOG(LTKLogger::LTK_LOGLEVEL_ERR)<<"Error: "<< errorCode << " " <<
            " NNShapeRecognizer::recognize()" << endl;
        LTKReturnError(errorCode);
    }
#endif
    //TODO?: If confThreshold is specified, get the entries from resultVector with confidence >= confThreshold

    // copy out results
    results.clear();

    shapeRecoResult_t res;

    for(int r=0; r<numChoices && r<eucDistIndexPairVector.size(); r++)
    {
        res.shapeID    = eucDistIndexPairVector[r].classId;
        res.confidence = eucDistIndexPairVector[r].confidence;

        results.push_back(res);
    }

    return true;
}

