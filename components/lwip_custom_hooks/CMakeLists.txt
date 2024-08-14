idf_component_register(SRCS "lwip_custom_hooks.c"
    INCLUDE_DIRS "include"
    REQUIRES lwip
)

# Solo aplicar estas configuraciones si no estamos en el contexto de la aplicación de prueba unitaria
if(NOT DEFINED PROJECT_VER)
    idf_component_get_property(lwip lwip COMPONENT_LIB)
    target_compile_options(${lwip} PRIVATE "-I${COMPONENT_DIR}/include")
    target_compile_definitions(${lwip} PRIVATE "-DESP_IDF_LWIP_HOOK_FILENAME=\"lwip_custom_hooks.h\"")
endif()