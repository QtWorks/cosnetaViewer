#ifndef VORECOGNITIONRESULT_H
#define VORECOGNITIONRESULT_H

#include <QString>
#include <QStringList>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonParseError>

#include "hwrTypes.h"

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

class VoRecognitionResult
{
public:
    VoRecognitionResult();

    void recognise(const QByteArray &resultAsJSON);

    QString                 instanceId();
    QString                 uniqueID();
    QStringList             shapes();
    QVector<shapeResult_t>  shapeResults();
    int                     sizeShapeResults();
    QStringList             tables();
    QStringList             groups();
    QStringList             textLines();

private:
    QString                 m_instanceId;
    QString                 m_uniqueID;
    QStringList             m_shapes;
    QVector<shapeResult_t>  m_shapeResults;
    QStringList             m_textLines;
    QStringList             m_tables;
    QStringList             m_groups;

};

#endif // VORECOGNITIONRESULT_H
