#pragma once

#include "ring_link_netif_common.h"

#ifdef __cplusplus
extern "C" {
#endif


#define RING_LINK_NETIF_RX_CONFIG()                  \
    {                                                 \
        .base = RING_LINK_NETIF_RX_BASE,      \
        .driver = NULL,                               \
        .stack = RING_LINK_NETIF_RX_NETSTACK, \
    }

#define RING_LINK_NETIF_TX_CONFIG()                  \
    {                                                 \
        .base = RING_LINK_NETIF_TX_BASE,      \
        .driver = NULL,                               \
        .stack = RING_LINK_NETIF_TX_NETSTACK, \
    }

#define RING_LINK_NETIF_RX_BASE        &_g_esp_netif_inherent_ring_link_config
#define RING_LINK_NETIF_RX_NETSTACK     &_g_esp_netif_netstack_ring_link_config

#define RING_LINK_NETIF_TX_BASE        &_g_esp_netif_inherent_ring_link_tx_config
#define RING_LINK_NETIF_TX_NETSTACK     &_g_esp_netif_netstack_ring_link_tx_config

extern const esp_netif_inherent_config_t _g_esp_netif_inherent_ring_link_config;
extern const esp_netif_netstack_config_t _g_esp_netif_netstack_ring_link_config;

extern const esp_netif_inherent_config_t _g_esp_netif_inherent_ring_link_tx_config;
extern const esp_netif_netstack_config_t _g_esp_netif_netstack_ring_link_tx_config;


#ifdef __cplusplus
}
#endif
