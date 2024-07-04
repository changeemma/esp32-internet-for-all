#pragma once

// C libraries
#include <stdio.h>
#include <stdint.h>

#include "config.h"
#include "spi_lowlevel.h"

#ifdef __cplusplus
extern "C" {
#endif


#define SPI_PAYLOAD_BUFFER_SIZE 40
#define SPI_PAYLOAD_TTL 4

typedef enum {
    SPI_PAYLOAD_TYPE_INTERNAL = 0,
    SPI_PAYLOAD_TYPE_ESP_NETIF = 1,
} spi_buffer_type_t;

typedef uint8_t spi_payload_id_t;

typedef struct
{
    spi_payload_id_t id;
    uint8_t ttl;
    device_id_t src_device_id;
    device_id_t dst_device_id;
    spi_buffer_type_t buffer_type;
    char buffer[ SPI_PAYLOAD_BUFFER_SIZE ];
    uint8_t len;
} spi_payload_t;

bool spi_payload_is_for_device(spi_payload_t *p);

bool spi_payload_is_from_device(spi_payload_t *p);

bool spi_payload_is_broadcast(spi_payload_t *p);

esp_err_t spi_payload_transmit(spi_payload_t *p);

esp_err_t spi_payload_receive(spi_payload_t *p);

esp_err_t spi_payload_forward(spi_payload_t *p);

#ifdef __cplusplus
}
#endif

