#ifndef __SPI_RX_NETIF_H
#define __SPI_RX_NETIF_H

#include "spi_netif.h"

ESP_EVENT_DECLARE_BASE(SPI_RX_EVENT);


esp_netif_t * get_spi_rx_netif(void);
esp_err_t spi_rx_netif_init(void);

#endif

