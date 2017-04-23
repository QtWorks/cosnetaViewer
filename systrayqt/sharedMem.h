#ifndef SHAREDMEM_H
#define SHAREDMEM_H

#error "This must be removed and a reference to the API library should be made."

#include <stdint.h>
#ifndef _WIN32
#include <stdbool.h>
#endif

#define SHARED_MEMORY_KEY          "cosnetaInputDataShare01"
#define SHARED_DATA_UPDATE_RATE_HZ 100
#define MAX_PENS                   8

typedef struct
{
    double   pos[3];
    double   vel[3];
    double   acc[3];
    double   rot[3];
    double   rot_rate[3];
    int      screenX;
    int      screenY;
    uint32_t buttons;               // Qt::LeftButton, ...
    uint32_t pen_id;
    uint32_t gesture_sequence_num;
    uint32_t gesture_id;
    uint32_t active;

} pen_state_t;

typedef struct
{
    pen_state_t  pen[MAX_PENS];
    uint32_t     message_number;        // Incremented by systray sender
    uint32_t     message_number_read;   // Incremented by the reader

} cosneta_devices_t;


#endif // SHAREDMEM_H
