#ifndef DONGLEDATA_H
#define DONGLEDATA_H

// For MAX_PENS
#include "../cosnetaapi/sharedMem.h"

#define HDR_START_SENTINAL     0x349A2301
#define HDR_END_SENTINAL       0x0033B622
#define RESET_COMMAND          0xDB33A20F
#define DANGEROUS_UPDATES      0xA0A63397
#define DEBUG_CONTROL          0xDA540113

typedef struct pen_sensor_data_s
{
    unsigned char  quaternion[16];
    unsigned char  ms_vel_x[2];
    unsigned char  ms_vel_y[2];
    unsigned char  accel[12];
    unsigned char  last_hub_message;
    unsigned char  buttons[2];

} pen_sensor_data_t;

typedef struct usb_sensor_data_set_s
{
    char              pen_present[MAX_PENS];
    pen_sensor_data_t pen_sensors[MAX_PENS];

} usb_sensor_data_set_t;

#endif // DONGLEDATA_H
