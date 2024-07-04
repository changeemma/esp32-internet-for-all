#ifndef __SPI_NETIF_H
#define __SPI_NETIF_H


#include <stdio.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_mac.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"

#include "lwip/err.h"
#include "lwip/sys.h"
#include "lwip/netif.h"
#include "esp_netif.h"
#include "netif/etharp.h"
#include "lwip/esp_netif_net_stack.h"
#include "esp_netif_net_stack.h"
#include "lwip/esp_pbuf_ref.h"
#include "arpa/inet.h" // for ntohs, etc.

#include "lwip/udp.h"
#include "lwip/stats.h"
#include "lwip/inet_chksum.h"

#include "spi_payload.h"
#include "config.h"
#include "utils.h"

ESP_EVENT_DECLARE_BASE(SPI_RX_EVENT);
ESP_EVENT_DECLARE_BASE(SPI_TX_EVENT);

typedef enum
{
    SPI_EVENT_START, /**< ESP32 soft-AP start */
    SPI_EVENT_STOP, /**< ESP32 soft-AP stop */
    SPI_EVENT_NEW_ROUTE, /**< ESP32 soft-AP stop */
} network_spi_event_t;

struct spi_netif_driver
{
    esp_netif_driver_base_t base; /*!< base structure reserved as esp-netif driver */
    esp_interface_t interface;    /*!< handle of driver implementation */
};

typedef struct spi_netif_driver *spi_netif_driver_t;

uint32_t get_spi_ip_v4_by_orientation(void);
uint32_t get_spi_gateway_v4_by_orientation(void);
esp_err_t spi_netif_attach(esp_netif_t * esp_netif, esp_err_t (*post_attach_callback)(esp_netif_t *, void *));
esp_err_t spi_netif_init(esp_netif_t **esp_netif, esp_netif_config_t *esp_netif_config, esp_err_t (*post_attach_callback)(esp_netif_t *, void *));

esp_err_t spi_netif_handler(spi_payload_t *p);
esp_netif_t *get_spi_tx_netif(void);
esp_err_t spi_netif_init_changeme(void);

#endif
