#include "ring_link_netif.h"

static const char* TAG = "==> ring_link_netif";


esp_err_t ring_link_netif_init(void)
{
    ESP_ERROR_CHECK(ring_link_rx_netif_init());
    ESP_ERROR_CHECK(ring_link_tx_netif_init());
    return ESP_OK;
}
