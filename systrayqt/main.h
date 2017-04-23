#ifndef MAIN_H
#define MAIN_H

#include <QString>
#include <QMutex>
#include "pen_location.h"
// #### NOTE MOVED include "gestures.h" to after decalration of pen_modes_t below ####

#define FREERUNNER_PID L"PID_612B"
#define FLIP_PID       L"PID_612A"
#define POC2_PID       L"PID_6129"


extern int  mouseSensitivity;
#ifdef DEBUG_TAB
extern bool generateSampleData;
extern bool useMouseData;
#endif
extern bool freerunnerPointerDriveOn;
extern bool freerunnerGesturesDriveOn;

void    balloonMessage(QString msg);
void    sendKeypressToOS(int keyWithModifier, bool withShift, bool withControl, bool withAlt);
void    sendWindowsKeyCombo(int keyWithModifier, bool withShift, bool withControl, bool withAlt);
void    sendApplicationCommandOS(int applicationType, int command);
void    setMouseButtons(QPoint *pos, int buttons);
void    msleep(unsigned long t);
bool    freeStyleIsRunning(void);
void    launchFreeStyle(void);
void    stopFreeStyle(void);
QString getCosnetadevicePath(void);
bool    willAutoStart(void);
void    setAutoStart(bool start);
void    quitProgram(void);

// Update obfuscator
extern quint32 XORPermutor;
char   nextXORByte(void);

typedef enum pen_modes_e
{
    NORMAL_OVERLAY=1,
    DRIVE_MOUSE,
    PRESENTATION_CONTROLLER

} pen_modes_t;

typedef enum application_type_e
{
    APP_PDF=10,     // Explicit as used on settings write
    APP_POWERPOINT

} application_type_t;

//#define AVAILABLE_UPDATES_ROOT "http://192.168.2.60/CosnetaUpdate/"
#define AVAILABLE_UPDATES_ROOT "https://s3-eu-west-1.amazonaws.com/cosneta-updates/"
#define AVAILABLE_UPDATES_URL        AVAILABLE_UPDATES_ROOT "Available.dat"
#define WIN_DATA_URL                 AVAILABLE_UPDATES_ROOT "WinUpdate.dat"
#define MAC_DATA_URL                 AVAILABLE_UPDATES_ROOT "OSXUpdate.dat"
#define LINUX_DATA_URL               AVAILABLE_UPDATES_ROOT "LinuxUpdate.dat"
#define LOCAL_DOWNLOAD_BLOB_FILENAME "download.dat"

#define VERSION_FILE "version.dat"


// #### ################# ####
// #### NOTE INCLUDE HERE ####
// #### ################# ####
#include "gestures.h"


typedef enum
{
    DRIVE_SYSTEM_WITH_LEAD_PEN=20,
    DRIVE_SYSTEM_WITH_ANY_LOCAL_PEN,
    DRIVE_SYSTEM_WITH_ANY_PEN,
    NO_SYSTEM_DRIVE

} system_drive_t;

#define MOUSE_DRIVE_STATE_STR(d) = \
    ((d)==DRIVE_SYSTEM_WITH_LEAD_PEN?"lead pen":\
     (d)==DRIVE_SYSTEM_WITH_ANY_LOCAL_PEN?"any local":\
     (d)==DRIVE_SYSTEM_WITH_ANY_PEN?"any pen":\
     (d)==NO_SYSTEM_DRIVE?"no drive":"INVALID")

typedef enum
{
    NO_LEAD_PEN=30,
    USE_FIRST_CONNECTED_LOCAL_PEN,
    USE_FIRST_CONNECTED_PEN,
    MANUAL_PEN_SELECT

} lead_select_policy_t;

#define LEAD_PEN_POLICY_STR(d) = \
    ((d)==MANUAL_PEN_SELECT?"manual pen select":\
     (d)==USE_FIRST_CONNECTED_LOCAL_PEN?"first local pen":\
     (d)==USE_FIRST_CONNECTED_PEN?"first pen":\
     (d)==NO_LEAD_PEN?"no drive":"INVALID")



// And the shared data that is resident in this module

typedef struct
{
    pen_state_t      pen[MAX_PENS];                         // Output data (shared by appComms)
    uint32_t         gesture_sequence_num[MAX_PENS];
    uint32_t         gesture_key[MAX_PENS];                 // Application control keypress
    uint32_t         gesture_key_mods[MAX_PENS];
    pen_settings_t   settings[MAX_PENS];                    // Output/input data
    bool             settings_changed_by_client[MAX_PENS];
    bool             settings_changed_by_systray[MAX_PENS];
    char             networkPenAddress[MAX_PENS][40];  // Space for IPv4 string or IPv6 string


    int              leadPen;

    enum pen_modes_e penMode[MAX_PENS];
    bool             gestureDriveActive[MAX_PENS];

    pen_location    *locCalc[MAX_PENS];            // Any location calculations required for this device type
    gestures         gestCalc[MAX_PENS];           // Gesture maintenence for this device type
    QMutex          *addRemoveDeviceMutex;
    bool             applicationIsConnected;       // Any application (can use this to turn off mouse drive etc)

} sysTray_devices_state_t;

#endif // MAIN_H
