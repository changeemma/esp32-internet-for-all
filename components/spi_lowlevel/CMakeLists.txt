idf_component_register(SRCS 
        "spi_lowlevel.c"
    INCLUDE_DIRS "include"
    REQUIRES driver device spi_payload
)