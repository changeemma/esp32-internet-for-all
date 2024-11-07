#pragma once

// C libraries
#include <stdio.h>
#include <stdint.h>

#include "config.h"
#include "ring_link_lowlevel_impl.h"

#ifdef __cplusplus
extern "C" {
#endif


// TCP/IP Headers
#define IP_HEADER_SIZE        20
#define TCP_HEADER_SIZE       20
#define MAX_PROTOCOL_HEADERS  (IP_HEADER_SIZE + TCP_HEADER_SIZE)  // 80 bytes

#define PADDING_SIZE(x) ((4 - ((x) % 4)) % 4)
#define RING_LINK_PAYLOAD_BUFFER_SIZE RING_LINK_LOWLEVEL_BUFFER_SIZE
#define BUFFER_SIZE (RING_LINK_PAYLOAD_BUFFER_SIZE + MAX_PROTOCOL_HEADERS)
#define BUFFER_PADDED_SIZE (BUFFER_SIZE + PADDING_SIZE(BUFFER_SIZE))

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
typedef enum: uint8_t {
    RING_LINK_PAYLOAD_TYPE_INTERNAL = 0x00,
    RING_LINK_PAYLOAD_TYPE_INTERNAL_HEARTBEAT = 0x01,
    
    RING_LINK_PAYLOAD_TYPE_ESP_NETIF = 0x80,
} ring_link_payload_buffer_type_t;

#define IS_HEARTBEAT_PAYLOAD(type) ((type) == RING_LINK_PAYLOAD_TYPE_INTERNAL_HEARTBEAT)

typedef uint8_t ring_link_payload_id_t;

typedef struct
{
    ring_link_payload_id_t id;
    uint8_t ttl;
    uint8_t padding1[2];
    config_id_t src_id;
    config_id_t dst_id;
    ring_link_payload_buffer_type_t buffer_type;
    uint8_t len;
    uint8_t padding2[2];
    char buffer[BUFFER_PADDED_SIZE];
} ring_link_payload_t;

bool ring_link_payload_is_for_device(ring_link_payload_t *p);

bool ring_link_payload_is_from_device(ring_link_payload_t *p);

bool ring_link_payload_is_broadcast(ring_link_payload_t *p);

bool ring_link_payload_is_internal(ring_link_payload_t *p);

bool ring_link_payload_is_heartbeat(ring_link_payload_t *p);

bool ring_link_payload_is_esp_netif(ring_link_payload_t *p);


#ifdef __cplusplus
}
#endif

