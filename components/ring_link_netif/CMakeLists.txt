idf_component_register(SRCS 
        "ring_link_netif_common.c"
        "ring_link_netif_rx.c"
        "ring_link_netif_tx.c"
        "ring_link_netif.c"
    INCLUDE_DIRS "include"
    REQUIRES esp_netif ring_link_lowlevel utils
)