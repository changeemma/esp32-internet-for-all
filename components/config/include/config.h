#ifndef __CONFIG_H
#define __CONFIG_H

#include "esp_log.h"
#include "driver/gpio.h"

#define CONFIG_GPIO_PIN_0 22 // the lowest bit
#define CONFIG_GPIO_PIN_1 21
#define CONFIG_GPIO_PIN_2 16 // the highest bit

typedef enum {
    ORIENTATION_NORTH = 0, // 000
    ORIENTATION_SOUTH = 1, // 001
    ORIENTATION_EAST = 2, // 010
    ORIENTATION_WEST = 3, // 011
    ORIENTATION_NONE = 9
} orientation_t;

typedef enum {
    BOARD_MODE_PEER_LINK = 0,
    BOARD_MODE_ACCESS_POINT = 4, // 100 - Wi-Fi AccessPoint
    BOARD_MODE_ROOT = 5, // 101
    BOARD_MODE_NONE = 9
} board_mode_t;

typedef struct {
    board_mode_t board_mode;
    orientation_t orientation;
} config_t;

void read_config(void);
bool read_pin(int);

orientation_t get_orientation(void);
board_mode_t get_board_mode(void);

#endif
