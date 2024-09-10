#include "ring_link.h"

static const char *TAG = "==> ring_link";

ring_link_handlers_t ring_link_handlers = {
    .internal_handler = ring_link_internal_handler,
    .netif_handler = ring_link_netif_handler
};

esp_err_t process_ring_link_payload(ring_link_payload_t *p)
{
    esp_err_t rc = ESP_OK;
    switch (p->buffer_type)
    {
    case RING_LINK_PAYLOAD_TYPE_INTERNAL:
    case RING_LINK_PAYLOAD_TYPE_INTERNAL_HEARTBEAT:
        rc = ring_link_handlers.internal_handler(p);
        break;
    case RING_LINK_PAYLOAD_TYPE_ESP_NETIF:
        rc = ring_link_handlers.netif_handler(p);
        break;
    default:
        ESP_LOGE(TAG, "Unknown payload type: '%i'", p->buffer_type);
        rc = ESP_FAIL;
        break;
    }
    return rc;
}
static void ring_link_receive_task(void *pvParameters)
{
    esp_err_t rc;
    ring_link_payload_t p;
    while (true) {
        memset(&p, 0, sizeof(ring_link_payload_t));
        rc = ring_link_lowlevel_receive_payload(&p);
        ESP_ERROR_CHECK_WITHOUT_ABORT(rc);
        if (rc != ESP_OK) continue;

        rc = process_ring_link_payload(&p);
        ESP_ERROR_CHECK_WITHOUT_ABORT(rc);
    }
}

esp_err_t ring_link_init(void)
{
    #ifdef CONFIG_RING_LINK_LOWLEVEL_IMPL_SPI
    printf("CONFIG_RING_LINK_LOWLEVEL_IMPL_SPI\n");
    #endif

    #ifdef CONFIG_RING_LINK_LOWLEVEL_IMPL_UART
    printf("CONFIG_RING_LINK_LOWLEVEL_IMPL_UART\n");
    #endif
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