#pragma once

#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

#include "ring_link_payload.h"
#include "ring_link_lowlevel.h"

#ifdef __cplusplus
extern "C" {
#endif

#define RING_LINK_INTERNAL_QUEUE_SIZE 5

bool broadcast_to_siblings(const void *msg, uint16_t len);

bool send_heartbeat(const void *msg, uint16_t len);

esp_err_t ring_link_internal_init(void);

esp_err_t ring_link_internal_handler(ring_link_payload_t *p);

#ifdef __cplusplus
}
#endif

