#ifndef HWRTYPES_H
#define HWRTYPES_H

#include <QVector>
#include <QString>

typedef struct NeighborInfo
{
    int   classId;
    float distance;
    float confidence;
    int   prototypeSetIndex;

} NeighborInfo_t;

typedef struct
{
    float  X;
    float  Y;
    float  sinTheta;
    float  cosTheta;
    bool   penUp;

} PointFloatShapeFeature_t;


typedef struct
{
    int                                  classID;
    QVector<PointFloatShapeFeature_t *>  samples;

} NNShapeSample_t;


typedef struct
{
    int   shapeID;
    float confidence;

} shapeRecoResult_t;

typedef QVector<shapeRecoResult_t> shapeRecoResultSet_t;

typedef QVector<PointFloatShapeFeature_t> PointFloatShape_t;

typedef QVector<float>         floatVector_t;

typedef struct
{
    float X;
    float Y;

} tracePoint_t;

typedef QVector<tracePoint_t> traceLine_t;

typedef struct
{
    QVector<traceLine_t> line;
    float                XScaleFactor,YScaleFactor; // Not yet used...
}
trace_t;

typedef enum { smoothenTraceGroup,  normalizeSize,  resampleTraceGroup} preprocessIDs;



#define PREPROC_DEF_TRACE_DIMENSION 60
#define PREPROC_DEF_RESAMPLINGMETHOD "lengthbased"
#define PREPROC_DEF_SIZE_THRESHOLD 0.01f
#define PREPROC_DEF_PRESERVE_ASPECT_RATIO true
#define PREPROC_DEF_ASPECTRATIO_THRESHOLD 3.0f
#define PREPROC_DEF_PRESERVE_RELATIVE_Y_POSITION false
#define PREPROC_DEF_SMOOTHFILTER_LENGTH 3
#define NN_DEF_PREPROC_SEQ		"{CommonPreProc::normalizeSize,CommonPreProc::resampleTraceGroup,CommonPreProc::normalizeSize}"
#define NN_DEF_PROTOTYPESELECTION "hier-clustering"
#define NN_DEF_PROTOTYPEREDUCTIONFACTOR -1
#define NN_DEF_FEATURE_EXTRACTOR "PointFloatShapeFeatureExtractor"
#define NN_DEF_DTWEUCLIDEANFILTER -1
#define NN_DEF_REJECT_THRESHOLD 0.001
#define NN_DEF_NEARESTNEIGHBORS 1
#define NN_DEF_PROTOTYPEDISTANCE "dtw"
#define NN_DEF_BANDING 0.33

//ActiveDTW  parameters
#define ACTIVEDTW_DEF_PERCENTEIGENENERGY 90
#define ACTIVEDTW_DEF_EIGENSPREADVALUE 16
#define ACTIVEDTW_DEF_USESINGLETON true
#define ACTIVEDTW_DEF_DTWEUCLIDEANFILTER 100

#define NEURALNET_DEF_NORMALIZE_FACTOR	10.0
#define NEURALNET_DEF_RANDOM_NUMBER_SEED	426
#define NEURALNET_DEF_LEARNING_RATE		0.5
#define NEURALNET_DEF_MOMEMTUM_RATE		0.25
#define NEURALNET_DEF_TOTAL_ERROR		0.00001
#define NEURALNET_DEF_INDIVIDUAL_ERROR	0.00001
#define NEURALNET_DEF_HIDDEN_LAYERS_SIZE	1
#define	NEURALNET_DEF_HIDDEN_LAYERS_UNITS	25
#define NEURALNET_DEF_MAX_ITR	100


#define PREPROC_DEF_NORMALIZEDSIZE 10.0f
#define PREPROC_DEF_DOT_THRESHOLD 0.01f
#define PREPROC_DEF_LOOP_THRESHOLD 0.25f
#define PREPROC_DEF_HOOKLENGTH_THRESHOLD1 0.17f
#define PREPROC_DEF_HOOKLENGTH_THRESHOLD2 0.33f
#define PREPROC_DEF_HOOKANGLE_THRESHOLD 30
#define PREPROC_DEF_QUANTIZATIONSTEP 5
#define PREPROC_DEF_FILTER_LENGTH 3
#define NN_NUM_CLUST_INITIAL -2

#define PREPROC_DEF_INTERPOINT_DIST_FACTOR 15

// LVQ parameters
#define NN_DEF_LVQITERATIONSCALE 40
#define NN_DEF_LVQINITIALALPHA 0.3
#define NN_DEF_LVQDISTANCEMEASURE "eu"

#define NN_DEF_MDT_UPDATE_FREQ 5
#define NN_DEF_RECO_NUM_CHOICES 2

#define NN_MDT_OPEN_MODE_BINARY "binary"
#define NN_MDT_OPEN_MODE_ASCII "ascii"



//ADAPT parameters
#define ADAPT_DEF_MIN_NUMBER_SAMPLES_PER_CLASS 5
#define ADAPT_DEF_MAX_NUMBER_SAMPLES_PER_CLASS 10


#define DEFAULT_SAMPLING_RATE   100
#define DEFAULT_X_DPI           2000
#define DEFAULT_Y_DPI           2000
#define DEFAULT_LATENCY         0.0


#define EPS 0.00001F


#define EUCLIDEAN_FILTER_OFF -1


#endif // HWRTYPES_H
