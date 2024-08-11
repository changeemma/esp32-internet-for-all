#pragma once

// C libraries
#include <stdio.h>
#include <stdint.h>

#include "config.h"
#include "ring_link_lowlevel_spi.h"

#ifdef __cplusplus
extern "C" {
#endif


#define RING_LINK_PAYLOAD_BUFFER_SIZE RING_LINK_LOWLEVEL_BUFFER_SIZE
#define RING_LINK_PAYLOAD_TTL 4

typedef enum {
    RING_LINK_PAYLOAD_TYPE_INTERNAL = 0,
    RING_LINK_PAYLOAD_TYPE_ESP_NETIF = 1,
} ring_link_payload_buffer_type_t;

typedef uint8_t ring_link_payload_id_t;

typedef struct
{
    ring_link_payload_id_t id;
    uint8_t ttl;
    device_id_t src_device_id;
    device_id_t dst_device_id;
    ring_link_payload_buffer_type_t buffer_type;
    char buffer[ RING_LINK_PAYLOAD_BUFFER_SIZE ];
    uint8_t len;
} ring_link_payload_t;

bool ring_link_payload_is_for_device(ring_link_payload_t *p);

bool ring_link_payload_is_from_device(ring_link_payload_t *p);

bool ring_link_payload_is_broadcast(ring_link_payload_t *p);

#ifdef __cplusplus
}
#endif

