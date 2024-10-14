#include "ring_link_netif_common.h"

static const char* TAG = "==> ring_link_netif_common";

uint32_t get_ring_link_rx_ip_v4_by_orientation(void) {
    config_orientation_t orientation = config_get_orientation();
    uint32_t ip = GET_RX_IP(orientation);
    return ip;
}

uint32_t get_ring_link_tx_ip_v4_by_orientation(void) {
    config_orientation_t orientation = config_get_orientation();
    uint32_t ip = GET_TX_IP(orientation);
    return ip;
}

uint32_t get_ring_link_rx_ip_v4_gateway_by_orientation(void) {
    config_orientation_t orientation = config_get_orientation();
    uint32_t ip = GET_RX_IP(orientation);
    return ip;
}

uint32_t get_ring_link_tx_ip_v4_gateway_by_orientation(void) {
    config_orientation_t orientation = config_get_orientation();
    uint32_t ip = GET_TX_IP(orientation);
    return ip;
}

esp_err_t ring_link_netif_esp_netif_attach(esp_netif_t * esp_netif, esp_err_t (*post_attach_callback)(esp_netif_t *, void *))
{

    ring_link_netif_driver_t driver = calloc(1, sizeof(struct ring_link_netif_driver));
    if (driver == NULL)
    {
        ESP_LOGE(TAG, "No memory to create a ring-link interface handle");
        return ESP_FAIL;
    }

    driver->base.post_attach = post_attach_callback;
    ESP_LOGI(TAG, "esp_ring_link_create_if_driver() called");
    
    if (driver == NULL)
    {
        ESP_LOGE(TAG, "Failed to create ring-link interface handle");
        return ESP_FAIL;
    }
    ESP_LOGI(TAG, "ring_link_netif_esp_netif_attach(esp_netif_t * esp_netif) called");

    return esp_netif_attach(esp_netif, driver);
}

esp_err_t ring_link_netif_esp_netif_init(esp_netif_t *esp_netif, esp_err_t (*post_attach_callback)(esp_netif_t *, void *))
{       
    ring_link_netif_esp_netif_attach(esp_netif, post_attach_callback);
    return ESP_OK;
}