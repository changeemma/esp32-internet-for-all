#include "spi_netif.h"

const char *SPI_NETIF_TAG = "==> spi_netif";

uint32_t get_spi_ip_v4_by_orientation(void){
    device_orientation_t orientation;
    orientation = device_config_get_orientation();
    return ESP_IP4TOADDR(127, 0, 0, (int) orientation + 1);
}

uint32_t get_spi_gateway_v4_by_orientation(void){
    device_orientation_t orientation;
    orientation = device_config_get_orientation();
    return ESP_IP4TOADDR(127, 0, 0, (int) orientation + 1);
}

esp_err_t spi_netif_attach(esp_netif_t * esp_netif, esp_err_t (*post_attach_callback)(esp_netif_t *, void *))
{

    spi_netif_driver_t driver = calloc(1, sizeof(struct spi_netif_driver));
    if (driver == NULL)
    {
        ESP_LOGE(SPI_NETIF_TAG, "No memory to create a spi interface handle");
        return NULL;
    }

    driver->base.post_attach = post_attach_callback;
    ESP_LOGI(SPI_NETIF_TAG, "esp_spi_create_if_driver() called");
    
    if (driver == NULL)
    {
        ESP_LOGE(SPI_NETIF_TAG, "Failed to create spi interface handle");
        return ESP_FAIL;
    }
    ESP_LOGI(SPI_NETIF_TAG, "esp_netif_attach_spi(esp_netif_t * esp_netif) called");

    return esp_netif_attach(esp_netif, driver);
}



esp_err_t spi_netif_init(esp_netif_t **esp_netif, esp_netif_config_t *esp_netif_config, esp_err_t (*post_attach_callback)(esp_netif_t *, void *))
{       
    *esp_netif = esp_netif_new(esp_netif_config);

    if (*esp_netif == NULL) {
        ESP_LOGE(SPI_NETIF_TAG, "esp_netif_new failed!");
    }

    struct netif *spi_netif_impl = esp_netif_get_netif_impl(*esp_netif);
    netif_set_flags(spi_netif_impl, NETIF_FLAG_BROADCAST);
    spi_netif_attach(*esp_netif, post_attach_callback);

    return ESP_OK;
}