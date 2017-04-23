#ifndef DONGLEREADER_H
#define DONGLEREADER_H

#include <QObject>
#include <QThread>
#include <QString>
#include <QFile>

#include "main.h"
#include "GUI.h"
#include "dongleData.h"
#include "build_options.h"


#ifdef _WIN32

#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
#include <windows.h>
#endif

class dongleReader : public QObject
{
    Q_OBJECT
public:
    explicit dongleReader(sysTray_devices_state_t *table, QObject *parent = 0);
    ~dongleReader();

    void    doConnect(QThread &cThread);
    void    stopCommand(void);
    void    startCommand(void);
    void    waitForStop(void);
    bool    readDebugRead(char *buf, int maxLen);

    void    setGuiPointer(class CosnetaGui *update);
    bool    updatePenSettings(int penNum,  pen_settings_t *settings);

signals:
    
public slots:
    void    readThread();

private:
    sysTray_devices_state_t *sharedTable;
    CosnetaGui              *guiPtr;
#ifdef NEED_SCSI_IOCTL
    bool                     readSensorDataIoctl(char *buffer, int maxLen);
#else
    bool                     readSensorDataFS(char *buffer, int maxLen);
#endif
    bool                     readSensorNativeOpen(void);
    bool                     readSensorNativeRead(char *buffer, int maxLen);
#ifdef NEED_SCSI_IOCTL
    bool                     readSettingsNativeData(int penNum, pen_settings_t *settings);
#else
    bool                     readSettingsDataFS(int penNum, pen_settings_t *settings);
#endif
    void                     fill_in_data(pen_state_t *out, pen_sensor_data_t *in);

    bool                     pen_was_present[MAX_PENS];
    int                      pen_map[MAX_PENS];

    bool                     quit;
    QFile                    dongleFile;
    bool                     nativeOpen;
#ifdef _WIN32
    HANDLE                   Handle;
#endif

};

#endif // DONGLEREADER_H
