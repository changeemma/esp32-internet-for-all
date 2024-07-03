#ifndef __PHYSICAL_SPI_H
#define __PHYSICAL_SPI_H

#include <stdio.h>
#include <string.h>

#include "esp_log.h"

#include "driver/spi_master.h"
#include "driver/spi_slave.h"
#include "lwip/ip4.h"

#include "utils.h"

#define SPI_QUEUE_SIZE 3

#define SENDER_GPIO_MOSI 23
#define SENDER_GPIO_SCLK 18
#define SENDER_GPIO_CS 5

#define RECEIVER_GPIO_MOSI 13
#define RECEIVER_GPIO_SCLK 14
#define RECEIVER_GPIO_CS 15

#define SPI_SENDER_HOST VSPI_HOST
#define SPI_RECEIVER_HOST HSPI_HOST

#define SPI_PAYLOAD_BUFFER_SIZE 800

esp_err_t spi_init(void);
esp_err_t spi_tx_init(void);
esp_err_t spi_rx_init(void);
void spi_transmit(void * buffer, size_t len);
size_t spi_receive(void *buffer);

#endif
