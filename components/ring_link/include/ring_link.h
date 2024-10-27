#pragma once

#include "ring_link_lowlevel.h"
#include "ring_link_internal.h"
#include "ring_link_netif.h"

#ifdef __cplusplus
extern "C" {
#endif

#define RING_LINK_MEM_TASK 16384
#define RING_LINK_NETIF_MEM_TASK 16384
#define RING_LINK_INTERNAL_MEM_TASK 8192
#define RING_LINK_READ_QUEUE_SIZE 20
#define RING_LINK_NETIF_QUEUE_SIZE 20

typedef struct {
    esp_err_t (*internal_handler)(ring_link_payload_t *p);
    esp_err_t (*netif_handler)(ring_link_payload_t *p);
} ring_link_handlers_t;

extern ring_link_handlers_t ring_link_handlers;

esp_err_t process_ring_link_payload(ring_link_payload_t *p);
esp_err_t ring_link_init(void);


#ifdef __cplusplus
}
#endif

