idf_component_register(SRC_DIRS "."
                       INCLUDE_DIRS "."
                       REQUIRES unity
                                freertos
                                esp_common
                                esp_system
                                ring_link_lowlevel_mock
                                ring_link_lowlevel
                                )