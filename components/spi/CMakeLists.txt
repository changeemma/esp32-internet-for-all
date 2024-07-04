idf_component_register(SRCS 
        "spi_lowlevel.c"
        "spi_payload.c"
        "spi_internal.c"
        "spi_netif.c"
        "udp_spi.c"
        "spi.c"
    INCLUDE_DIRS "include"
    PRIV_REQUIRES lwip_custom_hooks
    REQUIRES esp_netif esp_wifi driver utils device
)