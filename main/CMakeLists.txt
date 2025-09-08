set(MAIN_SRCS
    "main.c"
    "nvs.c"
    "test_spi.c"
)

set(MAIN_INCLUDE_DIRS
    "include"
)

set(MAIN_REQUIRES
    config
    utils
    ring_link
    heartbeat
    wifi
    nvs_flash
)

idf_component_register(
    SRCS
        ${MAIN_SRCS}
    INCLUDE_DIRS
        ${MAIN_INCLUDE_DIRS}
    REQUIRES
        ${MAIN_REQUIRES}
)
