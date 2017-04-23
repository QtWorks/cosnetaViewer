
#ifndef FREERUNNER_LOW_LEVEL_H
#define FREERUNNER_LOW_LEVEL_H

#include "build_options.h"

#include <QTextStream>

#include <stdint.h>
#ifdef __GNUC__
#include <stdbool.h>
#endif

// For the devs parameter in openHardwareDevice()

#define DEVICE_ACCEL1      0x0001
#define DEVICE_ACCEL2      0x0002
#define DEVICE_ANAGYRO     0x0004
#define DEVICE_DIGIGYRO    0x0008
#define DEVICE_TEMPSENSOR  0x0010
#define DEVICE_IMAGESENSOR 0x0020
#define DEVICE_ROTACCEL    0x0040
#define DEVICE_ROTGYRO     0x0080
#define DEVICE_QUARTRNIONS 0x0100
#define DEVICE_TAPDETECT   0x0200
#define DEVICE_DMP         0x0400

#define DEVICE_ALL         0x07FF


typedef struct
{
    int          accel1[3];
    int          accel2[3];
    int          angle_rate[3];
    unsigned int buttons;
    double       timeStep_accel1;
    double       timeStep_accel2;
    double       timeStep_angle_rate;

    double       quaternion[4];

} cosneta_sample_t;


// These functions match the signature of every operating system interface.
// Ifdefs exclude code for the wrong operationg system.

bool openHardwareDevice(void);
void closeHardwareDevice(void);
bool freerunnerDeviceOpened(void);
bool readSample( cosneta_sample_t *out);

#ifdef DEBUG_TAB

#include <QTextStream>

extern bool        dumpSamplesToFile;
extern QTextStream accel1TextOut;
extern QTextStream accel2TextOut;
extern QTextStream gyroTextOut;

bool startDataSaving(void);
void stopDataSaving(void);
void dumpSamples(QTextStream &out, double time, unsigned short data[], int numData);
void dumpUserInput(double time, unsigned char data[], int dataSize);

/* Commands used with write */

#define FR_CMD_SEND_WLESS    0x01
#define FR_CMD_GET_DEVICE_ID 0x02
#define FR_CMD_INIT_WLESS    0x03
#define FR_CMD_SET_RX_MAC    0x04
#define FR_CMD_WLESS_INTR    0x05
#define FR_CMD_WLESS_REG_WR  0x06
#define FR_CMD_WLESS_REG_RD  0x07

bool LowlevelWirelessDev_Read(char *buf, int buffSize);
bool LowlevelWirelessDev_Write(char cmd, char *buf, int buffSize);

#endif


#endif  // FREERUNNER_LOW_LEVEL_H

