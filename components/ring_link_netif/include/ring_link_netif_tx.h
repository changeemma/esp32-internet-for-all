#pragma once

#include "ring_link_netif_common.h"

#ifdef __cplusplus
extern "C" {
#endif


ESP_EVENT_DECLARE_BASE(RING_LINK_TX_EVENT);

esp_err_t ring_link_tx_netif_init(void);

esp_netif_t *get_ring_link_tx_netif(void);

err_t ring_link_tx_netif_netstack_init_fn(struct netif *netif);

esp_netif_recv_ret_t ring_link_tx_netif_netstack_input_fn(void *h, void *buffer, size_t len, void* l2_buff);

#ifdef __cplusplus
}
#endif
