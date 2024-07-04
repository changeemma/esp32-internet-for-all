#include "spi.h"

static const char* TAG = "==> spi";

static void spi_receive_task( void *pvParameters )
{
    esp_err_t rc;
    spi_payload_t p;
    while (true) {
        memset(&p, 0, sizeof(spi_payload_t));
        rc = spi_payload_receive(&p);
        ESP_ERROR_CHECK_WITHOUT_ABORT(rc);
        if (rc != ESP_OK) continue;

        switch (p.buffer_type)
        {
        case SPI_PAYLOAD_TYPE_INTERNAL:
            ESP_ERROR_CHECK_WITHOUT_ABORT(spi_internal_handler(&p));
            break;
        case SPI_PAYLOAD_TYPE_ESP_NETIF:
            // check if netif is up
            ESP_ERROR_CHECK_WITHOUT_ABORT(spi_netif_handler(&p));
            break;
        default:
            ESP_LOGE(TAG, "Unknown payload type: '%i'", p.buffer_type);
            break;
        }
    }
    vTaskDelete(NULL);
}

esp_err_t spi_init(void)
{
    spi_lowlevel_init();
    spi_internal_init();

    BaseType_t ret = xTaskCreate(spi_receive_task, "spi_receive_task", 2048, NULL, (tskIDLE_PRIORITY + 2), NULL);

    if (ret != pdTRUE) {
        ESP_LOGE(TAG, "create flow control task failed");
        return ESP_FAIL;
    }
    
    return ESP_OK;
}