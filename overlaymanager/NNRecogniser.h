#ifndef NNRECOGNISER_H
#define NNRECOGNISER_H

#include <QFile>
#include <QMap>
#include <QString>
#include <QVector>

#include "hwrTypes.h"
#include "genericPreProcessor.h"

#define TOKENIZE_DELIMITER	"<>=\n\r"

#define DELEMITER_FUNC "::"

#define HEADERLEN           "HEADERLEN"
#define FE_NAME	            "FE_NAME"
#define CKS					"CKS"
#define PROJNAME			"PROJNAME"
#define PROJTYPE			"PROJTYPE"
#define RECNAME				"RECNAME"
#define	RECVERSION			"RECVERSION"
#define CREATETIME			"CREATETIME"
#define MODTIME				"MODTIME"
#define PLATFORM			"PLATFORM"
#define COMMENTLEN			"COMMENTLEN"
#define DATAOFFSET			"DATAOFFSET"
#define NUMSHAPES			"NUMSHAPES"
#define PREPROC_SEQ         "PREPROC_SEQ"
#define SIZEOFINT			"SIZEOFINT"
#define SIZEOFUINT			"SIZEOFUINT"
#define SIZEOFSHORTINT		"SIZEOFSHORTINT"
#define SIZEOFFLOAT			"SIZEOFFLOAT"
#define SIZEOFCHAR			"SIZEOFCHAR"
#define HIDDENLAYERSUNIT	"HIDDENLAYERSUNIT"
#define DATASET				"DATASET"
#define COMMENT				"COMMENT"
#define BYTEORDER			"BYTEORDER"
#define OSVERSION			"OSVERSION"
#define PROCESSOR_ARCHITEC	"PROCESSOR_ARCHITEC"
#define HEADERVER			"HEADERVER"

class NNRecogniser
{
public:
    NNRecogniser();

    bool loadModelData(QString fileName);
    bool recognise(trace_t lines, shapeRecoResultSet_t &results);

private:
    bool  readHeader(QFile &inFile, QMap<QString,QString> &header);
    bool  tokeniseString(const QString& inputString, QVector<QString>& outTokens);
    bool  validateDataParams(void);
    bool  pointFloatShapeExtractFeatures(trace_t &traceGroup, PointFloatShape_t &outTrace);
    bool  computeEuclideanDistance(int protoNum, PointFloatShape_t features, float &euclideanDistance);
    float getDistance(PointFloatShapeFeature_t *a, PointFloatShapeFeature_t *b);

    int   m_dtwEuclideanFilter;
    int   m_nearestNeighbors;
    QMap<QString,QString>    dataParams;
    QVector<NNShapeSample_t> m_prototypeSet;
    QMap<int,int>            classFromIndex;

    genericPreProcessor      preprocess;
};

#endif // NNRECOGNISER_H
