#pragma once

#include "ring_link_internal.h"

#ifdef __cplusplus
extern "C" {
#endif

esp_err_t broadcast_init( void );
bool broadcast_to_siblings(const void *msg, uint16_t len);
esp_err_t ring_link_broadcast_handler(ring_link_payload_t *p);

#ifdef __cplusplus
}
#endif

