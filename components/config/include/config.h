#pragma once

#include "esp_log.h"
#include "driver/gpio.h"
#include "device.h"

#ifdef __cplusplus
extern "C" {
#endif

#define DEVICE_CONFIG_PIN_0 22 // the lowest bit
#define DEVICE_CONFIG_PIN_1 21
#define DEVICE_CONFIG_PIN_2 16 // the highest bit
#define DEVICE_CONFIG_PIN_MASK  ((1ULL<<DEVICE_CONFIG_PIN_0) | (1ULL<<DEVICE_CONFIG_PIN_1) | (1ULL<<DEVICE_CONFIG_PIN_2))

typedef struct {
    device_id_t id;
    device_mode_t mode;
    device_orientation_t orientation;
} device_config_t;


device_id_t device_config_get_id(void);

device_mode_t device_config_get_mode(void);

device_orientation_t device_config_get_orientation(void);

void device_config_setup(void);

void device_config_print(void);

#ifdef __cplusplus
}
#endif

