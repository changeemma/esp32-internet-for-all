idf_component_register(SRCS 
     "ring_link.c"
     "heartbeat.c"
    INCLUDE_DIRS "include"
    REQUIRES ring_link_lowlevel ring_link_internal ring_link_netif esp_timer
)