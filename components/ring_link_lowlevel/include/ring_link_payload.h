#pragma once

// C libraries
#include <stdio.h>
#include <stdint.h>

#include "config.h"
#include "ring_link_lowlevel_impl.h"

#ifdef __cplusplus
extern "C" {
#endif


#define RING_LINK_PAYLOAD_BUFFER_SIZE (RING_LINK_LOWLEVEL_BUFFER_SIZE - 8)
#define RING_LINK_PAYLOAD_TTL 4

/**
 * @brief Types of payloads in the ring link communication.
 *
 * This enumeration defines the various types of payloads that can be
 * transmitted in the ring link system. The values are carefully chosen
 * to distinguish between internal and external payloads.
 *
 * Internal payloads (< 0x80):
 * - Used for communication within the ring link system itself.
 * - Include types like regular internal messages and heartbeats.
 *
 * External payloads (>= 0x80):
 * - Used for payloads originating from or destined to external systems.
 * - Include types like ESP-NETIF messages.
 */
typedef uint16_t ring_link_payload_buffer_type_t;

enum {
    RING_LINK_PAYLOAD_TYPE_INTERNAL = 0x11,
    RING_LINK_PAYLOAD_TYPE_ESP_NETIF = 0x80,
};

typedef uint8_t ring_link_payload_id_t;

typedef struct __attribute__((packed, aligned(4))) {
    ring_link_payload_id_t id;                      // uint8_t
    ring_link_payload_buffer_type_t buffer_type;    // uint16_t
    uint16_t len;                                   // uint16_t
    uint8_t ttl;                                    // uint8_t
    config_id_t src_id;                             // uint8_t
    config_id_t dst_id;                             // uint8_t
    // uint8_t _reserved;                              // padding
    char buffer[RING_LINK_PAYLOAD_BUFFER_SIZE];
} ring_link_payload_t;

// Compile-time checks
_Static_assert((RING_LINK_PAYLOAD_BUFFER_SIZE % 4) == 0,
               "RING_LINK_PAYLOAD_BUFFER_SIZE must be a multiple of 4 for DMA compatibility");

_Static_assert((sizeof(ring_link_payload_t) % 4) == 0,
               "ring_link_payload_t is not 4-byte aligned");

_Static_assert(__alignof__(ring_link_payload_t) == 4,
               "ring_link_payload_t does not have 4-byte alignment");

bool ring_link_payload_is_for_device(ring_link_payload_t *p);

bool ring_link_payload_is_from_device(ring_link_payload_t *p);

bool ring_link_payload_is_broadcast(ring_link_payload_t *p);

bool ring_link_payload_is_internal(ring_link_payload_t *p);

bool ring_link_payload_is_esp_netif(ring_link_payload_t *p);


#ifdef __cplusplus
}
#endif

