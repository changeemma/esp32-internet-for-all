idf_component_register(SRCS "physical_spi.c" "spi_netif.c" "udp_spi.c" "spi_rx_netif.c" "spi_tx_netif.c"
    INCLUDE_DIRS "include"
    PRIV_REQUIRES lwip_custom_hooks
    REQUIRES esp_netif esp_wifi driver utils device
)