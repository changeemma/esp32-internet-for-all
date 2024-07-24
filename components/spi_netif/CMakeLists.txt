idf_component_register(SRCS 
        "spi_netif.c" 
        "udp_spi.c"
    INCLUDE_DIRS "include"
    PRIV_REQUIRES lwip_custom_hooks
    REQUIRES esp_netif spi_lowlevel utils
)