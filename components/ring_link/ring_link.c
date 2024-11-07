#include "ring_link.h"

static const char *TAG = "==> ring_link";

QueueHandle_t ring_link_queue = NULL;
QueueHandle_t ring_link_netif_queue = NULL;
QueueHandle_t ring_link_internal_queue = NULL;


esp_err_t process_ring_link_payload(ring_link_payload_t *p)
{
    QueueHandle_t specific_queue = NULL;

    if (ring_link_payload_is_internal(p) || ring_link_payload_is_heartbeat(p))
    {
        specific_queue = ring_link_internal_queue;
    }
    else if (ring_link_payload_is_esp_netif(p))
    {
        specific_queue = ring_link_netif_queue;
    }
    else
    {
        ESP_LOGE(TAG, "Unknown payload type: '%i'", p->buffer_type);
        return ESP_FAIL;
    }

    if (xQueueSend(specific_queue, &p, pdMS_TO_TICKS(100)) != pdTRUE) {
        ESP_LOGW(TAG, "Queue full, payload dropped");
        return ESP_FAIL;
    }
    return ESP_OK;
}

static esp_err_t ring_link_internal_handler(ring_link_payload_t *p)
{
    if (ring_link_payload_is_broadcast(p))  // broadcast
    {
        return ring_link_broadcast_handler(p);
    }
    else if (ring_link_payload_is_heartbeat(p))
    {
        return ring_link_heartbeat_handler(p);
    }
    else if (ring_link_payload_is_for_device(p))  // payload for me
    {
        return ring_link_process(p);
    }
    else  // not for me, forwarding
    {
        return ring_link_lowlevel_forward_payload(p);        
    }
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
        }
    }
}

static void ring_link_internal_process_task(void *pvParameters)
{
    ring_link_payload_t *payload;
    esp_err_t rc;
    
    while (true) {
        if (xQueueReceive(ring_link_internal_queue, &payload, portMAX_DELAY) == pdTRUE) {
            rc = ring_link_internal_handler(payload);
            ESP_ERROR_CHECK_WITHOUT_ABORT(rc);
            free(payload);
        }
    }
}

static void ring_link_netif_process_task(void *pvParameters)
{
    ring_link_payload_t *payload;
    esp_err_t rc;
    
    while (true) {
        if (xQueueReceive(ring_link_netif_queue, &payload, portMAX_DELAY) == pdTRUE) {
            rc = ring_link_netif_handler(payload);
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

    ring_link_queue = xQueueCreate(RING_LINK_READ_QUEUE_SIZE, sizeof(ring_link_payload_t*));
    ring_link_internal_queue = xQueueCreate(RING_LINK_INTERNAL_QUEUE_SIZE, sizeof(ring_link_payload_t*));
    ring_link_netif_queue = xQueueCreate(RING_LINK_NETIF_QUEUE_SIZE, sizeof(ring_link_payload_t*));
    
    if (ring_link_queue == NULL || ring_link_internal_queue == NULL || ring_link_netif_queue == NULL) {
        ESP_LOGE(TAG, "Failed to create queues");
        return ESP_FAIL;
    }

    ESP_ERROR_CHECK(ring_link_lowlevel_init());
    ESP_ERROR_CHECK(ring_link_internal_init());
    ESP_ERROR_CHECK(ring_link_netif_init());
    broadcast_init();
    BaseType_t ret;
    
    ret = xTaskCreate(
        ring_link_receive_task,
        "ring_link_receive",
        RING_LINK_MEM_TASK,
        NULL,
        (tskIDLE_PRIORITY + 4),
        NULL
    );
    if (ret != pdTRUE) {
        ESP_LOGE(TAG, "Failed to create receive task");
        return ESP_FAIL;
    }

    ret = xTaskCreate(
        ring_link_process_task,
        "ring_link_process",
        RING_LINK_NETIF_MEM_TASK,
        NULL,
        (tskIDLE_PRIORITY + 4),
        NULL
    );
    if (ret != pdTRUE) {
        ESP_LOGE(TAG, "Failed to create process task");
        return ESP_FAIL;
    }

    ret = xTaskCreate(
        ring_link_internal_process_task,
        "ring_link_internal_process",
        RING_LINK_INTERNAL_MEM_TASK,
        NULL,
        (tskIDLE_PRIORITY + 2),
        NULL
    );
    if (ret != pdTRUE) {
        ESP_LOGE(TAG, "Failed to create internal process task");
        return ESP_FAIL;
    }

    ret = xTaskCreate(
        ring_link_netif_process_task,
        "ring_link_netif_process",
        RING_LINK_MEM_TASK,
        NULL,
        (tskIDLE_PRIORITY + 4),
        NULL
    );
    if (ret != pdTRUE) {
        ESP_LOGE(TAG, "Failed to create netif process task");
        return ESP_FAIL;
    }
    
    return ESP_OK;
}