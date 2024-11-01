#include "ring_link_internal.h"

static const char* TAG = "==> ring_link_internal";
static TaskHandle_t s_heartbeat_task = NULL;
static SemaphoreHandle_t s_broadcast_semaphore_handle = NULL;
static QueueHandle_t s_broadcast_queue = NULL;
static ring_link_payload_id_t s_id_counter = 0;

esp_err_t ring_link_internal_init( void )
{
    s_broadcast_semaphore_handle = xSemaphoreCreateMutex();
    if( s_broadcast_semaphore_handle == NULL )
    {
        ESP_LOGE(TAG, "an error ocurred creating mutex.");
        return ESP_FAIL;
    }

    s_broadcast_queue = xQueueCreate(RING_LINK_INTERNAL_QUEUE_SIZE, sizeof(ring_link_payload_id_t));
    if( s_broadcast_queue == NULL )
    {
        ESP_LOGE(TAG, "an error ocurred creating queue.");
        return ESP_FAIL;
    }
    return ESP_OK;
}


static esp_err_t ring_link_process(ring_link_payload_t *p)
{
    // printf("call on_sibling_message(%s, %i)\n", p->buffer, p->len);
    return ESP_OK;
}

static esp_err_t ring_link_broadcast(const void *buffer, uint16_t len){
    ring_link_payload_t p = {
        .id = s_id_counter ++,
        .ttl = RING_LINK_PAYLOAD_TTL,
        .src_id = config_get_id(),
        .dst_id = CONFIG_ID_ALL,
        .buffer_type = RING_LINK_PAYLOAD_TYPE_INTERNAL,
        .len = len,
    };
    if (len > RING_LINK_PAYLOAD_BUFFER_SIZE) {
        ESP_LOGE(TAG, "Buffer length exceeds maximum allowed size.");
        return ESP_ERR_INVALID_SIZE;
    }
    memcpy(p.buffer, buffer, len);
    return ring_link_lowlevel_transmit_payload(&p);
}

bool broadcast_to_siblings(const void *msg, uint16_t len)
{
    if( xSemaphoreTake( s_broadcast_semaphore_handle, ( TickType_t ) 100 ) == pdTRUE )
    {
        esp_err_t rc = ring_link_broadcast(msg, len);
        uint8_t id;
        bool result = false;
        if( xQueueReceive( s_broadcast_queue, &( id ), ( TickType_t ) 100 ) == pdPASS )
        {
            result = (rc == ESP_OK);
        }
        xSemaphoreGive( s_broadcast_semaphore_handle );
        return result;
    }
    ESP_LOGE(TAG, "Could not adquire Mutex...");
    return false;
}

static esp_err_t ring_link_broadcast_handler(ring_link_payload_t *p)
{
    // broadcast origin
    if (ring_link_payload_is_from_device(p))
    {
        xQueueSend(s_broadcast_queue, (void *) &(p->id), ( TickType_t ) 0 );
        ESP_LOGD(TAG, "Broadcast complete (src=%i,dest=%i,id=%i,ttl=%i).", p->src_id, p->dst_id, p->id, p->ttl);
        return ESP_OK;
    }
    else
    {
        ESP_ERROR_CHECK(ring_link_process(p));
        return ring_link_lowlevel_forward_payload(p);
    }
}

bool send_heartbeat(const void *msg, uint16_t len){
    ring_link_payload_t p = {
        .id = s_id_counter ++,
        .ttl = RING_LINK_PAYLOAD_TTL,
        .src_id = config_get_id(),
        .dst_id = config_get_id(),
        .buffer_type = RING_LINK_PAYLOAD_TYPE_INTERNAL_HEARTBEAT,
        .len = len,
    };
    if (len > RING_LINK_PAYLOAD_BUFFER_SIZE) {
        ESP_LOGE(TAG, "Buffer length exceeds maximum allowed size.");
        return ESP_ERR_INVALID_SIZE;
    }
    memcpy(p.buffer, msg, len);
    s_heartbeat_task = xTaskGetCurrentTaskHandle();
    ESP_ERROR_CHECK(ring_link_lowlevel_transmit_payload(&p));
    if( ulTaskNotifyTake( pdTRUE, ( TickType_t ) 100 ) == pdTRUE )
    {
        return true;
    }
    return false;
}

static esp_err_t ring_link_heartbeat_handler(ring_link_payload_t *p)
{
    // broadcast origin
    if (ring_link_payload_is_from_device(p))
    {
        ESP_LOGD(TAG, "Heartbeat complete (src=%i,dest=%i,id=%i,ttl=%i).", p->src_id, p->dst_id, p->id, p->ttl);
        xTaskNotifyGive( s_heartbeat_task );
        return ESP_OK;
    }
    else
    {
        return ring_link_lowlevel_forward_payload(p);
    }
}


esp_err_t ring_link_internal_handler(ring_link_payload_t *p)
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
