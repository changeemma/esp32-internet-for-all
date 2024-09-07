#pragma once

#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

#include "ring_link_payload.h"
#include "ring_link_lowlevel.h"

#ifdef __cplusplus
extern "C" {
#endif

bool broadcast_to_siblings(const void *msg, uint16_t len);

bool broadcast_to_siblings_heartbeat(const void *msg, uint16_t len);

bool ring_link_send_heartbeat(const void *msg, uint16_t len);

esp_err_t ring_link_internal_init(void);

esp_err_t ring_link_internal_handler(ring_link_payload_t *p);

#ifdef __cplusplus
}
#endif

