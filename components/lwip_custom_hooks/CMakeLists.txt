idf_component_register(SRCS "lwip_custom_hooks.c"
    INCLUDE_DIRS "include"
)

idf_component_get_property(lwip lwip COMPONENT_LIB)
target_compile_options(${lwip} PRIVATE "-I${PROJECT_DIR}/components/lwip_custom_hooks/include")
target_compile_definitions(${lwip} PRIVATE "-DESP_IDF_LWIP_HOOK_FILENAME=\"lwip_custom_hooks.h\"")
set_property(TARGET ${lwip} PROPERTY SOURCES ${PROJECT_DIR}/components/lwip_custom_hooks/lwip_custom_hooks.c APPEND)
