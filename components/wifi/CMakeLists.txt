idf_component_register(SRCS "wifi.c"
    INCLUDE_DIRS "include"
    REQUIRES config utils esp_wifi esp_event
)