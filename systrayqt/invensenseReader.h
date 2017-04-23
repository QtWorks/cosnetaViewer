#include "build_options.h"
#ifdef INVENSENSE_EV_ARM

#ifndef INVENSENSEREADER_H
#define INVENSENSEREADER_H

#include <QObject>
#include "main.h"
#include "invensense_low_level.h"

class invensenseReader : public QObject
{
    Q_OBJECT

public:
    invensenseReader(sysTray_devices_state_t *table, QObject *parent = 0);
    ~invensenseReader();

    void doConnect(QThread &cThread);
    void stopCommand(void);
    void startCommand(int port);
    void waitForStop(void);

public slots:
    void readThread();

private:
    bool                     closeDeviceCmd;
    bool                     portOpen;
    int                      portNum;
    sysTray_devices_state_t *sharedTable;
};


#endif // INVENSENSEREADER_H

#endif
