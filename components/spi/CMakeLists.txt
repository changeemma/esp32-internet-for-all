idf_component_register(SRCS 
        "spi_lowlevel.c"
        "spi_payload.c"
    INCLUDE_DIRS "include"
    REQUIRES driver device
)