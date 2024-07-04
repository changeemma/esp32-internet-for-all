#pragma once

#include "spi_payload.h"

#ifdef __cplusplus
extern "C" {
#endif

bool broadcast_to_siblings(const void *msg, uint16_t len);

esp_err_t spi_internal_init( void );

esp_err_t spi_internal_handler(spi_payload_t *p);

#ifdef __cplusplus
}
#endif

