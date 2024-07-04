idf_component_register(SRCS 
        "spi_internal.c"
    INCLUDE_DIRS "include"
    REQUIRES esp_netif esp_wifi driver utils device spi
)