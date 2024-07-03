#ifndef __SPI_TX_NETIF_H
#define __SPI_TX_NETIF_H

#include "spi_netif.h"

ESP_EVENT_DECLARE_BASE(SPI_TX_EVENT);


esp_netif_t * get_spi_tx_netif(void);
esp_err_t spi_tx_netif_init(void);

#endif

