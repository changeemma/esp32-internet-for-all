#pragma once

#include "ring_link_netif_common.h"

#ifdef __cplusplus
extern "C" {
#endif


ESP_EVENT_DECLARE_BASE(RING_LINK_TX_EVENT);

esp_err_t ring_link_tx_netif_init(void);
esp_netif_t *get_ring_link_tx_netif(void);

#ifdef __cplusplus
}
#endif
