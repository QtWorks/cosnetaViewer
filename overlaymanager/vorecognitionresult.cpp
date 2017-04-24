#include <QtCore>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonValue>
#include <QJsonParseError>

#include "vorecognitionresult.h"
#include "hwrTypes.h"

typedef struct {

    QString     instanceId;

    void printAll() {
        qDebug() << "=== Start of responseData_t ===========================";
        qDebug() << "instanceId: " << instanceId;
        qDebug() << "=== End of responseData_t =============================";
    }

} responseData_t;

typedef struct {

    QString                         uniqueID;
    QStringList                     shapesStringList;         // List of shapes identified just for debug
    QStringList                     textLines;
    QStringList                     tables;
    QStringList                     groups;
    QVector<shapeResult_t>          shapeResults;   // A collection of shape results (e.g. more than one shape)

    void printAll() {
        qDebug() << "=== Start of resultData_t ===========================";
        qDebug() << "uniqueID: " << uniqueID;
        qDebug() << "shapes: " << shapesStringList;
        qDebug() << "textLines: " << textLines;
        qDebug() << "tables: " << tables;
        qDebug() << "groups: " << groups;
        qDebug() << "=== End of resultData_t =============================";
    }

} resultData_t;


VoRecognitionResult::VoRecognitionResult()
{
}

void VoRecognitionResult::recognise( const QByteArray &resultAsJSON )
{
    QJsonParseError *err = new QJsonParseError();
    QJsonDocument doc = QJsonDocument::fromJson(resultAsJSON, err);

    if (err->error != 0)
        qDebug() << err->errorString();

    responseData_t responseData; // instanceId
    resultData_t resultData; // uniqueID, shapes etc

    // Write out the response to a file so I can store it
    /**
    QFile jsonFileOut("E:\\Dev\\Qjson_Qt5\\json_being_received.json");
    if (!jsonFileOut.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        qDebug() << "Unable to open jsonFileOut.json, exiting...";
        exit(1);
    }
    QTextStream out(&jsonFileOut);
    out << resultAsJSON;
    jsonFileOut.close();
    **/

    if (doc.isNull())
    {
        qDebug() << "Invalid JSON document. Exiting...";
        //return 1006;
    }
    else if (doc.isObject())
    {
        // Setup some map and list objects
        QVariantMap mainMap;
        QVariantMap resultMap;
        QVariantList shapesList;
        QVariantMap shapesMap;
        QVariantList shapeCandidatesList;
        QVariantMap shapeCandidatesMap;
        QVariantList shapePrimitivesList;
        QVariantMap shapePrimitivesMap;
        QVariantList shapePrimitivesFirstPointList;
        QVariantMap shapePrimitivesFirstPointMap;
        QVariantList shapePrimitivesLastPointList;
        QVariantMap shapePrimitivesLastPointMap;
        QVariantList candidatesList;
        QVariantMap candidatesMap;
        QVariantList textLinesList;
        QVariantMap textLinesMap;
        QVariantMap textLinesResultMap;
        QVariantMap textSegmentResultMap;
        QVariantList textSegmentResultCandidatesList;
        QVariantMap textSegmentResultCandidatesMap;
        QVariantList groupsList;
        QVariantList tablesList;

        //Get the jsonObject
        QJsonObject jObject = doc.object();
        //Convert the json object to variantmap
        mainMap = jObject.toVariantMap();
        responseData.instanceId = mainMap["instanceId"].toString();
        m_instanceId = responseData.instanceId;

        //Get the variant map for results
        resultMap = mainMap["result"].toMap();
        resultData.uniqueID = resultMap["uniqueID"].toString();
        m_uniqueID = resultData.uniqueID;

        // Handle shapes here
        shapesList = resultMap["shapes"].toList();
        m_shapeResults.clear();

        if (shapesList.size()>0)
        {
            // Set up structs to store shape results
            shapeResult_t shapeResult; // This is a container for all the shapes suggested by the VO web service
            shapeSingleResult_t shapeSingleResult; // This is a container for teh deatils of the individual shapes returned

            // Iterate over all the elements
            QVariantList::Iterator shapesListIter = shapesList.begin();
            while (shapesListIter != shapesList.end())
            {
                shapesMap = (*shapesListIter).toMap();
                shapeCandidatesList = shapesMap["candidates"].toList();

                QVariantList::Iterator shapeCandidatesListIter = shapeCandidatesList.begin();
                while (shapeCandidatesListIter != shapeCandidatesList.end()) {

                    shapeCandidatesMap = (*shapeCandidatesListIter).toMap();
                    shapeSingleResult.shapeCandidateType.append(shapeCandidatesMap["type"].toString());
                    shapeSingleResult.shapeName.append(shapeCandidatesMap["label"].toString());
                    shapeSingleResult.confidence = shapeCandidatesMap["resemblanceScore"].toFloat();

                    // This is just to keep visible track of the shapes identified
                    resultData.shapesStringList.append(shapeCandidatesMap["label"].toString());

                    // Debug
                    //qDebug() << "=====================";
                    //qDebug() << "1 candidateType is " << shapeCandidatesMap["type"].toString();
                    //qDebug() << "2 label is " << shapeCandidatesMap["label"].toString();
                    //qDebug() << "3 resemblanceScore is " << shapeCandidatesMap["resemblanceScore"].toFloat();

                    // Now need to dive into primitives & iterate to get x,y values
                    shapePrimitivesList = shapeCandidatesMap["primitives"].toList();
                    float x_point, y_point;
                    QPoint firstPoint, lastPoint;

                    //qDebug() << "   ===Primitives==================";

                    QVariantList::Iterator shapePrimitivesListIter = shapePrimitivesList.begin();
                    while (shapePrimitivesListIter != shapePrimitivesList.end()) {
                        shapePrimitivesMap = (*shapePrimitivesListIter).toMap();
                        shapeSingleResult.primitiveType.append(shapePrimitivesMap["type"].toString());

                        //qDebug() << "   1 type is " << shapePrimitivesMap["type"].toString();

                        shapePrimitivesFirstPointMap = shapePrimitivesMap["firstPoint"].toMap();
                        x_point = shapePrimitivesFirstPointMap["x"].toFloat();
                        y_point = shapePrimitivesFirstPointMap["y"].toFloat();
                        firstPoint.setX(x_point);
                        firstPoint.setY(y_point);
                        shapeSingleResult.firstPoint.append(firstPoint);

                        shapePrimitivesLastPointMap = shapePrimitivesMap["lastPoint"].toMap();
                        x_point = shapePrimitivesLastPointMap["x"].toFloat();
                        y_point = shapePrimitivesLastPointMap["y"].toFloat();
                        lastPoint.setX(x_point);
                        lastPoint.setY(y_point);
                        shapeSingleResult.lastPoint.append(lastPoint);

                        //qDebug() << "   firstPoint is " << shapePrimitivesFirstPointMap["x"].toFloat() << shapePrimitivesFirstPointMap["y"].toFloat();
                        //qDebug() << "   lastPoint is " << shapePrimitivesLastPointMap["x"].toFloat() << shapePrimitivesLastPointMap["y"].toFloat();

                        ++shapePrimitivesListIter;
                    }

                    // Add singleShapeResult to shapeResult (which is a vector)
                    shapeResult.shapeNames.append(shapeSingleResult);

                    // Clear the singleShapeResult_t data for this shape
                    shapeSingleResult.shapeCandidateType.clear();
                    shapeSingleResult.shapeName.clear();
                    shapeSingleResult.confidence = 0.0;
                    shapeSingleResult.primitiveType.clear();
                    shapeSingleResult.firstPoint.clear();
                    shapeSingleResult.lastPoint.clear();

                    ++shapeCandidatesListIter;
                }

                m_shapes = resultData.shapesStringList; // Keep (easily visible) track of the candidate shapes

                // Add shapeResult into the m_shapeResults struct so it can be called from handwritingRecognition.cpp
                m_shapeResults.append(shapeResult);
                //resultData.shapeResults.append(shapeResult);

                // Go on to the next shape (il.e. more than one shape has been recorded)
                ++shapesListIter;

                qDebug() << "SHAPES: Num of shape candidates found: " << shapeResult.shapeNames.size();
            }            

            qDebug() << "SHAPES: Total num of shapes found: " << resultData.shapeResults.size();
        }

        // Handle text (textLines) here
        textLinesList = resultMap["textLines"].toList();
        if (textLinesList.size()>0)
        {
            // Iterate over all the elements
            QVariantList::Iterator textLinesListIter = textLinesList.begin();
            while (textLinesListIter != textLinesList.end())
            {
                textLinesMap = (*textLinesListIter).toMap();
                //qDebug() << "textLinesMap.size() is " << textLinesMap.size();
                //qDebug() << "textLinesMap key/value pairs are: ";
                //for(QVariantMap::const_iterator iter = textLinesMap.begin(); iter != textLinesMap.end(); ++iter) {
                    //qDebug() << "Key: " << iter.key() << " -- Value:" << iter.value() << "\n";
                //}
                textLinesResultMap = textLinesMap["result"].toMap();
                //qDebug() << "textLinesResultMap.size() is " << textLinesResultMap.size();
                //qDebug() << "textLinesResultMap key/value pairs are: ";
                //for(QVariantMap::const_iterator iter = textLinesResultMap.begin(); iter != textLinesResultMap.end(); ++iter) {
                    //qDebug() << "Key: " << iter.key() << " -- Value:" << iter.value() << "\n";
                //}
                textSegmentResultMap = textLinesResultMap["textSegmentResult"].toMap();
                //qDebug() << "textSegmentResultMap.size() is " << textSegmentResultMap.size();
                //qDebug() << "textSegmentResultMap key/value pairs are: ";
                //for(QVariantMap::const_iterator iter = textSegmentResultMap.begin(); iter != textSegmentResultMap.end(); ++iter) {
                    //qDebug() << "Key: " << iter.key() << " -- Value:" << iter.value() << "\n";
                //}
                textSegmentResultCandidatesList = textSegmentResultMap["candidates"].toList();
                // Iterate over all the textSegmentResultCandidatesList elements (each contains a QVariantMap)
                QVariantList::Iterator it = textSegmentResultCandidatesList.begin();
                while (it != textSegmentResultCandidatesList.end()) {
                    textSegmentResultCandidatesMap = (*it).toMap();
                    //qDebug() << "textSegmentResultCandidatesMap.size() is " << textSegmentResultCandidatesMap.size();
                    //qDebug() << "textSegmentResultCandidatesMap key/value pairs are: ";
                    //for(QVariantMap::const_iterator iter = textSegmentResultCandidatesMap.begin(); iter != textSegmentResultCandidatesMap.end(); ++iter) {
                        //qDebug() << "Key: " << iter.key() << " -- Value:" << iter.value() << "\n";
                    //}
                    // Get the textLine
                    //qDebug() << "The recognised text is" << textSegmentResultCandidatesMap["label"].toString()
                    //         << "with resemblanceScore" << textSegmentResultCandidatesMap["resemblanceScore"].toString();

                    // Could use selectedCandidateIdx to locate index of selectedCandidate (d'oh!)
                    //qDebug() << "selectedCandidateIdx: " << textSegmentResultMap["selectedCandidateIdx"].toString();

                    resultData.textLines.append(textSegmentResultCandidatesMap["label"].toString());
                    ++it;
                }
                ++textLinesListIter;
            }
        }        
        m_textLines = resultData.textLines;

        // Handle groups here
        groupsList = resultMap["groups"].toList();
        if (groupsList.size()>0)
        {
            //resultData.groupsList = variantListToStringList(groupsList);

            //qDebug() << "\n=============\n";
            //qDebug() << "groupsList is " << groupsList << "\nDone.";
        }

        // Handle tables here
        tablesList = resultMap["tables"].toList();
        if (tablesList.size()>0)
        {
            //resultData.tablesList = variantListToStringList(tablesList);

            //qDebug() << "\n=============\n";
            //qDebug() << "tablesList is " << tablesList << "\nDone.";
        }

        //Print out the attributes of the response & result structs
        //responseData.printAll();
        //resultData.printAll();

        mainMap.clear();
        resultMap.clear();
        shapesList.clear();
        shapesMap.clear();
        shapeCandidatesList.clear();
        shapeCandidatesMap.clear();
        shapePrimitivesList.clear();
        shapePrimitivesMap.clear();
        shapePrimitivesFirstPointList.clear();
        shapePrimitivesFirstPointMap.clear();
        shapePrimitivesLastPointList.clear();
        shapePrimitivesLastPointMap.clear();
        candidatesList.clear();
        candidatesMap.clear();
        textLinesList.clear();
        textLinesMap.clear();
        textLinesResultMap.clear();
        textSegmentResultMap.clear();
        textSegmentResultCandidatesList.clear();
        textSegmentResultCandidatesMap.clear();
        groupsList.clear();
        tablesList.clear();
    }
    else if (doc.isArray())
    {
        qDebug() << "Are you sure this is the JSON from your web service? Exiting...";
        //return 1007;
    }
    else if (doc.isEmpty())
    {
        qDebug() << "Empty JSON document. Exiting...";
        //return 1008;
    }
    else
    {
        qDebug() << "Unable to determine type of JSON. Exiting...";
        //return 1009;
    }
}

QString VoRecognitionResult::instanceId()
{
    return m_instanceId;
}

QString VoRecognitionResult::uniqueID()
{
    return m_uniqueID;
}

QStringList VoRecognitionResult::shapes()
{
    return m_shapes;
}

QVector<shapeResult_t> VoRecognitionResult::shapeResults()
{
    return m_shapeResults;
}

int VoRecognitionResult::sizeShapeResults()
{
    return m_shapeResults.size();
}

QStringList VoRecognitionResult::tables()
{
    return m_tables;
}

QStringList VoRecognitionResult::groups()
{
    return m_groups;
}

QStringList VoRecognitionResult::textLines()
{
    return m_textLines;
}
