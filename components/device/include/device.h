#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    DEVICE_ORIENTATION_NORTH = 0, // 000
    DEVICE_ORIENTATION_SOUTH = 1, // 001
    DEVICE_ORIENTATION_EAST  = 2, // 010
    DEVICE_ORIENTATION_WEST  = 3, // 011
    DEVICE_ORIENTATION_NONE  = 9,
} device_orientation_t;

typedef enum {
    DEVICE_MODE_PEER_LINK    = 0,
    DEVICE_MODE_ACCESS_POINT = 4, // 100 - Wi-Fi AccessPoint
    DEVICE_MODE_ROOT         = 5, // 101
    DEVICE_MODE_NONE         = 9,
} device_mode_t;

typedef enum {
    DEVICE_ID_NORTH   = 0, // 000
    DEVICE_ID_SOUTH   = 1, // 001
    DEVICE_ID_EAST    = 2, // 010
    DEVICE_ID_WEST    = 3, // 011
    DEVICE_ID_CENTER  = 4, // 100
    DEVICE_ID_NONE    = 9,
    DEVICE_ID_ANY     = 10,
    DEVICE_ID_ALL     = 11,
} device_id_t;


#ifdef __cplusplus
}
#endif

