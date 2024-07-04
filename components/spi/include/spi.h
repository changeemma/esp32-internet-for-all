#pragma once

// C libraries
#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_check.h"

#include "spi_lowlevel.h"
#include "spi_internal.h"
#include "spi_netif.h"

#ifdef __cplusplus
extern "C" {
#endif

esp_err_t spi_init(void);
esp_err_t spi_rx_netif_init(void);
esp_err_t spi_tx_netif_init(void);

#ifdef __cplusplus
}
#endif

