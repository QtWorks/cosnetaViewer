#ifndef SHAREDMEM_H
#define SHAREDMEM_H

#include <stdint.h>
#ifndef _WIN32
#include <stdbool.h>
#endif

#define SHARED_MEMORY_KEY          "cosnetaInputDataShare01"

#define SHARED_DATA_UPDATE_RATE_HZ 100
#define MAX_PENS                   8

#include "cosnetaAPI.h"

typedef struct
{
    pen_state_t    pen[MAX_PENS];
    uint32_t       message_number;        // Incremented by systray sender
    uint32_t       message_number_read;   // Incremented by the reader

    pen_settings_t settings[MAX_PENS];
    uint32_t       settings_changed_by_client[MAX_PENS];
    uint32_t       settings_changed_by_systray[MAX_PENS];

    char           networkPenAddress[MAX_PENS][40];  // Space for IPv4 string or IPv6 string

} cosneta_devices_t;

#endif // SHAREDMEM_H
