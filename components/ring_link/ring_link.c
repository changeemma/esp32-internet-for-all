#include "ring_link.h"

static const char *TAG = "==> ring_link";


static void ring_link_receive_task( void *pvParameters )
{
    esp_err_t rc;
    ring_link_payload_t p;
    while (true) {
        memset(&p, 0, sizeof(ring_link_payload_t));
        rc = ring_link_lowlevel_receive_payload(&p);
        ESP_ERROR_CHECK_WITHOUT_ABORT(rc);
        if (rc != ESP_OK) continue;

        switch (p.buffer_type)
        {
        case RING_LINK_PAYLOAD_TYPE_INTERNAL:
            ESP_ERROR_CHECK_WITHOUT_ABORT(ring_link_internal_handler(&p));
            break;
        case RING_LINK_PAYLOAD_TYPE_ESP_NETIF:
            ESP_ERROR_CHECK_WITHOUT_ABORT(ring_link_netif_handler(&p));
            break;
        default:
            ESP_LOGE(TAG, "Unknown payload type: '%i'", p.buffer_type);
            break;
        }
    }
    vTaskDelete(NULL);
}

esp_err_t ring_link_init(void)
{
    ESP_ERROR_CHECK(ring_link_lowlevel_init());
    ESP_ERROR_CHECK(ring_link_internal_init());
    ESP_ERROR_CHECK(ring_link_netif_init());

    BaseType_t ret = xTaskCreate(ring_link_receive_task, "ring_link_receive_task", 2048, NULL, (tskIDLE_PRIORITY + 2), NULL);

    if (ret != pdTRUE) {
        ESP_LOGE(TAG, "create flow control task failed");
        return ESP_FAIL;
    }
    
    return ESP_OK;
}