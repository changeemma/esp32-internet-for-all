#pragma once

#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

#include "ring_link_payload.h"
#include "ring_link_lowlevel.h"

#ifdef __cplusplus
extern "C" {
#endif

esp_err_t ring_link_internal_init(void);
esp_err_t ring_link_process(ring_link_payload_t *p);
#ifdef __cplusplus
}
#endif

