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
    ring_link_payload_t *payload;
    
    while (true) {
        payload = heap_caps_malloc(sizeof(ring_link_payload_t), MALLOC_CAP_DEFAULT);
        if (payload == NULL) {
            ESP_LOGE(TAG, "Failed to allocate memory for payload");
            continue;
        }
        
        memset(payload, 0, sizeof(ring_link_payload_t));
        rc = ring_link_lowlevel_receive_payload(payload);
        ESP_ERROR_CHECK_WITHOUT_ABORT(rc);
        if (rc == ESP_OK) {
            if (xQueueSend(ring_link_queue, &payload, pdMS_TO_TICKS(100)) != pdTRUE) {
                ESP_LOGW(TAG, "Queue full, payload dropped");
                free(payload);
            }
        } else {
            free(payload);
        }
    }
}

static void ring_link_process_task(void *pvParameters)
{
    ring_link_payload_t *payload;
    esp_err_t rc;
    
    while (true) {
        if (xQueueReceive(ring_link_queue, &payload, portMAX_DELAY) == pdTRUE) {
            rc = process_ring_link_payload(payload);
            ESP_ERROR_CHECK_WITHOUT_ABORT(rc);
            
            free(payload);
        }
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

    ring_link_queue = xQueueCreate(RING_LINK_QUEUE_SIZE, sizeof(ring_link_payload_t*));
    if (ring_link_queue == NULL) {
        ESP_LOGE(TAG, "Failed to create queue");
        return ESP_FAIL;
    }

    ESP_ERROR_CHECK(ring_link_lowlevel_init());
    ESP_ERROR_CHECK(ring_link_internal_init());
    ESP_ERROR_CHECK(ring_link_netif_init());

    BaseType_t ret = xTaskCreate(
        ring_link_receive_task,
        "ring_link_receive",
        RING_LINK_MEM_TASK,
        NULL,
        (tskIDLE_PRIORITY + 3),
        NULL
    );

    if (ret != pdTRUE) {
        ESP_LOGE(TAG, "Failed to create receive task");
        return ESP_FAIL;
    }

    ret = xTaskCreate(
        ring_link_process_task,
        "ring_link_process",
        RING_LINK_MEM_TASK,
        NULL,
        (tskIDLE_PRIORITY + 2),
        NULL
    );

    if (ret != pdTRUE) {
        ESP_LOGE(TAG, "Failed to create process task");
        return ESP_FAIL;
    }
    
    return ESP_OK;
}