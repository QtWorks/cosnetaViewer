
#ifndef _FREERUNNER_READER_H
#define  _FREERUNNER_READER_H

#include <QObject>
#include "main.h"
#include "freerunner_low_level.h"

class FreerunnerReader : public QObject
{
    Q_OBJECT
    
public:
    FreerunnerReader(sysTray_devices_state_t *table, QObject *parent = 0);
    ~FreerunnerReader();

    void   doConnect(QThread &cThread);
    void   stopCommand(void);
    void   startCallibration(void);
    void   endCallibration(void);

    bool poc1_read_enabled;

public slots:
    void readThread();

private:
    void callibrateSample(cosneta_sample_t *sample);
    void accumulateStaticCallibration(cosneta_sample_t *sample);

#ifdef DEBUG_TAB
    int  mouseButtons;
#endif

    bool quitRequest;
    bool callibrating;

    double analogueGyroZeroPoint[3];
    double analogueGyroScale[3];
    double accelerometerZeroPoint[2][3];
    double accelerometerScale[2][3];

    double accelSum[2][3];
    double gyroSum[3];
    int    numSamples;

    sysTray_devices_state_t *sharedTable;
};

#endif
