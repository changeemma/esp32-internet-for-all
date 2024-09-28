#pragma once

#include "ring_link_netif_common.h"

#ifdef __cplusplus
extern "C" {
#endif


ESP_EVENT_DECLARE_BASE(RING_LINK_RX_EVENT);

esp_err_t ring_link_rx_netif_init(esp_netif_config_t *cfg);
esp_err_t ring_link_rx_netif_receive(ring_link_payload_t *p);

#ifdef __cplusplus
}
#endif
