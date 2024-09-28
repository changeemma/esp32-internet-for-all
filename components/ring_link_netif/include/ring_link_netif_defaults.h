#pragma once

#include "ring_link_netif_common.h"

#ifdef __cplusplus
extern "C" {
#endif


#define RING_LINK_NETIF_RX_CONFIG()                  \
    {                                                 \
        .base = RING_LINK_NETIF_BASE,      \
        .driver = NULL,                               \
        .stack = RING_LINK_NETIF_NETSTACK, \
    }

#define RING_LINK_NETIF_BASE        &_g_esp_netif_inherent_ring_link_config
#define RING_LINK_NETIF_NETSTACK     &_g_esp_netif_netstack_ring_link_config

extern const esp_netif_inherent_config_t _g_esp_netif_inherent_ring_link_config;
extern const esp_netif_netstack_config_t _g_esp_netif_netstack_ring_link_config;


#ifdef __cplusplus
}
#endif
