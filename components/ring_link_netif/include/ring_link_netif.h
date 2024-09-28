#pragma once

#include "ring_link_netif_rx.h"
#include "ring_link_netif_tx.h"

#ifdef __cplusplus
extern "C" {
#endif


esp_err_t ring_link_netif_init(void);

esp_err_t ring_link_netif_handler(ring_link_payload_t *p);

#ifdef __cplusplus
}
#endif
