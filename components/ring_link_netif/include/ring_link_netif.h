#pragma once

#include "ring_link_netif_rx.h"
#include "ring_link_netif_tx.h"

#ifdef __cplusplus
extern "C" {
#endif

#define RING_LINK_NETIF_MEM_TASK 16384
#define RING_LINK_NETIF_QUEUE_SIZE 40


esp_err_t ring_link_netif_init(QueueHandle_t **queue);

esp_err_t ring_link_netif_handler(ring_link_payload_t *p);

#ifdef __cplusplus
}
#endif
