idf_component_register(SRCS "wifi.c"
    INCLUDE_DIRS "include"
    REQUIRES device utils esp_wifi lwip_custom_hooks esp_event
)