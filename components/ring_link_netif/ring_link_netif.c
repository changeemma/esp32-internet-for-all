#include "ring_link_netif.h"

static const char* TAG = "==> ring_link_netif";
static QueueHandle_t ring_link_netif_queue = NULL;
#include "esp_timer.h"


static esp_err_t ring_link_netif_handler(ring_link_payload_t *p)
{
    if (ring_link_payload_is_for_device(p))  // payload for me
    {
        return ring_link_rx_netif_receive(p);
    }
    else  // not for me, warn and discard
    {
        ESP_LOGW(TAG, "Discarding packet. id '%i' ('%s').", p->id, p->buffer);
        return ESP_OK;        
    }
}

static void ring_link_netif_process_task(void *pvParameters)
{
    ring_link_payload_t *payload;
    esp_err_t rc;
    
    while (true) {
        if (xQueueReceive(ring_link_netif_queue, &payload, portMAX_DELAY) == pdTRUE) {
            int64_t transaction_start_time_ = esp_timer_get_time();
            rc = ring_link_netif_handler(payload);
            int64_t end_time = esp_timer_get_time();
            ESP_ERROR_CHECK_WITHOUT_ABORT(rc);
            int64_t duration = end_time - transaction_start_time_;
            ESP_LOGW(TAG, "Mensaje ring_link_netif_process_task: en %lld μs", duration);
            free(payload);
        }
    }
}

esp_err_t ring_link_netif_init(QueueHandle_t **queue)
{
    ESP_ERROR_CHECK(ring_link_rx_netif_init());
    ESP_ERROR_CHECK(ring_link_tx_netif_init());

    ring_link_netif_queue = xQueueCreate(RING_LINK_NETIF_QUEUE_SIZE, sizeof(ring_link_payload_t*));
    
    if (ring_link_netif_queue == NULL) {
        ESP_LOGE(TAG, "Failed to create queues");
        return ESP_FAIL;
    }
    *queue = &ring_link_netif_queue;

    BaseType_t ret = xTaskCreate(
        ring_link_netif_process_task,
        "ring_link_netif_process",
        RING_LINK_NETIF_MEM_TASK,
        NULL,
        (tskIDLE_PRIORITY + 10),
        NULL
    );
    if (ret != pdTRUE) {
        ESP_LOGE(TAG, "Failed to create netif process task");
        return ESP_FAIL;
    }

    return ESP_OK;
}
