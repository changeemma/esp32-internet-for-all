idf_component_register(SRCS "wifi.c"
    INCLUDE_DIRS "include"
    PRIV_REQUIRES device utils lwip_custom_hooks esp_event esp_wifi
)