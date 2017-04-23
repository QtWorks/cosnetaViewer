#ifndef TRAYCOMMS_H
#define TRAYCOMMS_H

#include <QObject>
#include <QSharedMemory>
#include <QMutex>
#include "sharedMem.h"

class trayComms : public QObject
{
    Q_OBJECT

public:
    explicit trayComms(QSharedMemory *share, QObject *parent = 0);
    ~trayComms();

    void doConnect(QThread &cThread);
    bool stopCommand(void);             // Return true is it did stop
    bool setMaxRate(int maxRateHz);
    void setOnlyCallbackOnChanges(bool onlyCallOnChanges);
    void setCallback(void (*callback)(pen_state_t *pen, int activePensMask));

private slots:
    void readThread();

private:
    void updateOverlayWithAllUserInputs(void);

    bool   quitRequest;
    bool   onlyCallOnChanges;
    int    minUpdateTimeMs;
    void (*registered_callback)(pen_state_t *pen, int numPens);

    cosneta_devices_t  prevState;
    int                prevActivePenMask;
    QSharedMemory     *externalShare;
    cosneta_devices_t *externalData;
    QMutex             inUserCode;
};

#endif // TRAYCOMMS_H
