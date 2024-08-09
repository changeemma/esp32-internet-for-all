#include "ring_link_netif_common.h"

static const char* TAG = "==> ring_link_netif_common";


uint32_t get_ring_link_ip_v4_by_orientation(void){
    device_orientation_t orientation;
    orientation = device_config_get_orientation();
    return ESP_IP4TOADDR(127, 0, 0, (int) orientation + 1);
}

uint32_t get_ring_link_gateway_v4_by_orientation(void){
    device_orientation_t orientation;
    orientation = device_config_get_orientation();
    return ESP_IP4TOADDR(127, 0, 0, (int) orientation + 1);
}

static esp_err_t ring_link_netif_esp_netif_attach(esp_netif_t * esp_netif, esp_err_t (*post_attach_callback)(esp_netif_t *, void *))
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



esp_err_t ring_link_netif_esp_netif_init(esp_netif_t **esp_netif, esp_netif_config_t *esp_netif_config, esp_err_t (*post_attach_callback)(esp_netif_t *, void *))
{       
    *esp_netif = esp_netif_new(esp_netif_config);

    if (*esp_netif == NULL) {
        ESP_LOGE(TAG, "esp_netif_new failed!");
    }

    struct netif *ring_link_netif_impl = esp_netif_get_netif_impl(*esp_netif);
    netif_set_flags(ring_link_netif_impl, NETIF_FLAG_BROADCAST);
    ring_link_netif_esp_netif_attach(*esp_netif, post_attach_callback);

    return ESP_OK;
}