#ifndef COSNETAAPI_GLOBAL_H
#define COSNETAAPI_GLOBAL_H

#ifdef __cplusplus
extern "C" {
#endif
#include <stdint.h>

// Some operating systems (yes, that means you, windows) require you to tag exported variables.

#ifdef COSNETA_LIBRARY_BUILD
#define LIB_INTERFACE Q_DECL_EXPORT
#else
// Don't use Qt defines for other folks benefit!
#ifdef _WIN32
#define LIB_INTERFACE __declspec(dllimport)
#else
#define LIB_INTERFACE extern
#endif
#endif

// Total number of devices (pens + network connections) available
#define MAX_PENS                   8

// Data block for a pen

// Button bits
#define BUTTON_LEFT_MOUSE         0x0001
#define BUTTON_RIGHT_MOUSE        0x0002
#define BUTTON_MIDDLE_MOUSE       0x0010
#define BUTTON_SCROLL_UP          0x0004
#define BUTTON_SCROLL_DOWN        0x0008
#define BUTTON_MODE_SWITCH        0x0020
#define BUTTON_OPT_A              0x0040
#define BUTTON_OPT_B              0x0080
#define BUTTON_TIP_NEAR_SURFACE   0x8000

#define KEY_NUM_NO_KEY            0xFFFFFFFF
#define KEY_MOD_SHIFT             0x01
#define KEY_MOD_CTRL              0x02

#define APP_CTRL_PEN_MODE                 0xC00000
#define APP_CTRL_MENU_COMMAND             0xB00000
#define APP_CTRL_RAW_KEYPRESS             0xA00000
#define APP_CTRL_IS_MENU_COMMAND(x)       (((x) & 0xF00000) == APP_CTRL_MENU_COMMAND )
#define APP_CTRL_IS_RAW_KEYPRESS(x)       (((x) & 0xFF0000) == APP_CTRL_RAW_KEYPRESS )
#define APP_CTRL_IS_PEN_MODE(x)           (((x) & 0xFF0000) == APP_CTRL_PEN_MODE )
#define APP_CTRL_PACKAGE_MENUCMD(key)     (((key) & 0xFF0FFFFF)  | APP_CTRL_MENU_COMMAND)
#define APP_CTRL_PACKAGE_KEYPRESS(key)    (((key) & 0xFF00FFFF)  | APP_CTRL_RAW_KEYPRESS)
#define APP_CTRL_PACKAGE_PENMODE(mode)    (((mode) & 0xFF00FFFF) | APP_CTRL_PEN_MODE)
#define APP_CTRL_MODKEY_FROM_RAW_KEYPRESS(x) ((x) &   0xFF00FFFF)
#define APP_CTRL_KEY_FROM_RAW_KEYPRESS(x)    ((x) &   0x0100FFFF)
#define APP_CTRL_MENU_CMD_FROM_RX(x)         ((x) &   0xFF0FFFFF)
#define APP_CTRL_PEN_MODE_FROM_RX(x)         ((x) &   0xFF00FFFF)

#define APP_CTRL_APP_TYPE_POWERPOINT 0xC00001

#define APP_CTRL_NO_ACTION           0xD00000
#define APP_CTRL_START_PRESENTATION  0xD00001
#define APP_CTRL_EXIT_PRESENTATION   0xD00002
#define APP_CTRL_NEXT_PAGE           0xD00003
#define APP_CTRL_PREV_PAGE           0xD00004
#define APP_CTRL_SHOW_HIGHLIGHT      0xD00005

#define APP_CTRL_START_SHORTCUT      0xD00020

#define APP_CTRL_PEN_MODE_TEST        0x0000
#define APP_CTRL_PEN_MODE_OVERLAY     0x0001
#define APP_CTRL_PEN_MODE_PRESNTATION 0x0002
#define APP_CTRL_PEN_MODE_MOUSE       0x0003
#define APP_CTRL_PEN_MODE_LOCAL       0x0004

typedef struct pen_state_s
{
    double   pos[3];
    double   vel[3];
    double   acc[3];
    double   rot[3];                // Roll, pitch, yaw
    double   rot_rate[3];
    int32_t  screenX;
    int32_t  screenY;
    uint32_t buttons;               // Bitfield of buttons
    uint32_t pen_id;
    uint32_t app_ctrl_sequence_num;
    uint32_t app_ctrl_action;
    uint32_t status_flags;

} pen_state_t;

// Defines that work on a pen_state_t:
#define COS_PEN_INACTIVE                0x00
#define COS_PEN_ACTIVE                  0x01
#define COS_PEN_FROM_NETWORK            0x02
#define COS_PEN_ALLOW_REMOTE_VIEW       0x04

#define COS_MODE_NORMAL             0
#define COS_MODE_DRIVE_MOUSE        1
#define COS_MODE_DRIVE_PRESENTATION 2
// #define COS_PEN_MODE???:         3,4,5,6,7

#define COS_PEN_MODE_STR(m)  ((m)==COS_MODE_NORMAL?"normal":\
                              (m)==COS_MODE_DRIVE_MOUSE?"mouse":\
                              (m)==COS_MODE_DRIVE_PRESENTATION?"presentation":"INVALID");

#define COS_PEN_SET_ALL_FLAGS(pst,flgs)   (pst).status_flags = (flgs)
#define COS_PEN_IS_ACTIVE(pst)            ((COS_PEN_ACTIVE            & (pst).status_flags) != 0)
#define COS_PEN_IS_FROM_NETWORK(pst)      ((COS_PEN_FROM_NETWORK      & (pst).status_flags) != 0)
#define COS_PEN_CAN_VIEW(pst)             ((COS_PEN_ALLOW_REMOTE_VIEW & (pst).status_flags) != 0)
#define COS_SET_PEN_MODE(ps,mode)         (ps).status_flags = (((ps).status_flags&~(7<<5)) | (((mode)&7)<<5))
#define COS_GET_PEN_MODE(pst)             ((((pst).status_flags)>>5) & 7)

#define MAX_USER_NAME_SZ            64
#define NUM_MAPPABLE_BUTTONS        16

typedef struct pen_settings_s
{
    char          users_name[MAX_USER_NAME_SZ];       // Identity string displayed when requesting to join presentation.
    unsigned char colour[4];                          // Current pen colour (RGB, pad)
    qint32        sensitivity;                        // Simple gain for mouse type actions
    char          button_order[NUM_MAPPABLE_BUTTONS]; // Allow users to map the pen buttons to each other
//    quint32       options_flags;

} pen_settings_t;



// The library ensures that a consistent set of data is availble. This
// data is copied locally into the library in this process.

// NB We use truth returned in an int rather than bool for maximum compatibility.
#define COS_BOOL          int
#define COS_TRUE            1
#define COS_FALSE           0

#define NET_PEN_ADDRESS_SZ 40

#define NET_PEN_CTRL_PORT  14753

// Event use (update type in a minute). Only a single callback is allowed.
LIB_INTERFACE COS_BOOL cos_register_pen_event_callback(void (*callback)(pen_state_t *pen, int activePensMask), int maxRateHz, COS_BOOL onlyOnChanged);
LIB_INTERFACE COS_BOOL cos_deregister_callback(void);
// For polling use
LIB_INTERFACE COS_BOOL cos_get_pen_data(int penNum, pen_state_t* state);    // return code for range check (this for C, remember).
// Settings interface
LIB_INTERFACE COS_BOOL cos_retreive_pen_settings(int pen_num, pen_settings_t *settings);
LIB_INTERFACE COS_BOOL cos_update_pen_settings(int pen_num, pen_settings_t *settings);
LIB_INTERFACE COS_BOOL cos_get_net_pen_addr(int penNum, char *address, COS_BOOL *wantsView); // NET_PEN_ADDRESS_SZ
// General tests
LIB_INTERFACE int      cos_num_connected_pens(void);            // quick return of number of connected pens (-1 on error)
LIB_INTERFACE COS_BOOL cos_API_is_running(void);                // Return true is shared memory can be accessed.

#ifdef __cplusplus
}
#endif

#endif // COSNETAAPI_GLOBAL_H
