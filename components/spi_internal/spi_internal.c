#include "spi_internal.h"

static const char* TAG = "==> spi-internal";
static SemaphoreHandle_t s_broadcast_semaphore_handle = NULL;
static QueueHandle_t s_broadcast_queue = NULL;

esp_err_t spi_internal_init( void )
{
    s_broadcast_semaphore_handle = xSemaphoreCreateMutex();
    if( s_broadcast_semaphore_handle == NULL )
    {
        ESP_LOGE(TAG, "an error ocurred creating mutex.");
        return ESP_FAIL;
    }

    s_broadcast_queue = xQueueCreate(1, sizeof(spi_payload_id_t));
    if( s_broadcast_queue == NULL )
    {
        ESP_LOGE(TAG, "an error ocurred creating queue.");
        return ESP_FAIL;
    }
    return ESP_OK;
}


static esp_err_t spi_internal_process(spi_payload_t *p)
{
    printf("call on_sibling_message(%s, %i)\n", p->buffer, p->len);
    return ESP_OK;
}

static esp_err_t spi_internal_broadcast(const void *buffer, uint16_t len){
    spi_payload_t p = {
        .id = 0,
        .ttl = SPI_PAYLOAD_TTL,
        .src_device_id = device_config_get_id(),
        .dst_device_id = DEVICE_ID_ALL,
        .buffer_type = SPI_PAYLOAD_TYPE_INTERNAL,
        .len = len,
    };
    memccpy(p.buffer, buffer, len, SPI_PAYLOAD_BUFFER_SIZE);
    return spi_payload_transmit(&p);
}

bool broadcast_to_siblings(const void *msg, uint16_t len)
{
    if( xSemaphoreTake( s_broadcast_semaphore_handle, ( TickType_t ) 10 ) == pdTRUE )
    {
        esp_err_t rc = spi_internal_broadcast(msg, len);
        uint8_t id;
        if( xQueueReceive( s_broadcast_queue, &( id ), ( TickType_t ) 10 ) == pdPASS )
        {
            xSemaphoreGive( s_broadcast_semaphore_handle );
            return rc == ESP_OK ? true : false;
        }
        
        return false;
    }
    ESP_LOGE(TAG, "Could not adquire Mutex...");
    return false;
}

static esp_err_t spi_internal_broadcast_handler(spi_payload_t *p)
{
    // broadcast origin
    if (spi_payload_is_from_device(p))
    {
        xQueueSend(s_broadcast_queue, (void *) &(p->id), ( TickType_t ) 0 );
        ESP_LOGI(TAG, "Broadcast complete for packet id '%i'.", p->id);
        return ESP_OK;
    }
    else
    {
        ESP_ERROR_CHECK(spi_internal_process(p));
        return spi_payload_forward(p);
    }
}

esp_err_t spi_internal_handler(spi_payload_t *p)
{
    if (spi_payload_is_broadcast(p))  // broadcast
    {
        return spi_internal_broadcast_handler(p);
    }
    else if (spi_payload_is_for_device(p))  // payload for me
    {
        return spi_internal_process(p);
    }
    else  // not for me, forwarding
    {
        return spi_payload_forward(p);        
    }
}
