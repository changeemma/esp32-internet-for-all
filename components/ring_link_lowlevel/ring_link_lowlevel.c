#include "ring_link_lowlevel.h"

static const char* TAG = "==> ring_link_lowlevel";

static SemaphoreHandle_t s_tx_semaphore_handle = NULL;


esp_err_t ring_link_lowlevel_init(void) {
    ESP_ERROR_CHECK(ring_link_lowlevel_init_impl());
    s_tx_semaphore_handle = xSemaphoreCreateMutex();
    return ESP_OK;
}

esp_err_t ring_link_lowlevel_transmit_payload(ring_link_payload_t *p)
{
    esp_err_t rc;
    if( xSemaphoreTake( s_tx_semaphore_handle, ( TickType_t ) 10 ) == pdTRUE )
    {
        rc = ring_link_lowlevel_transmit_impl(p, sizeof(ring_link_payload_t));
        xSemaphoreGive( s_tx_semaphore_handle );
        return rc;
    }
    ESP_LOGE(TAG, "Could not adquire Mutex...");
    return ESP_FAIL;
}

esp_err_t ring_link_lowlevel_receive_payload(ring_link_payload_t *p)
{
    return ring_link_lowlevel_receive_impl(p, sizeof(ring_link_payload_t));
}

esp_err_t ring_link_lowlevel_forward_payload(ring_link_payload_t *p)
{
    if (p->ttl <= 0) {
        ESP_LOGW(TAG, "Discarding packet id '%i' ('%s') due to TTL. Won't forward.", p->id, p->buffer);
        return ESP_OK;
    }
    p->ttl--;
    ESP_LOGI(TAG, "Forwarding packet id '%i' with dest '%i' (TTL=%i).", p->id, p->dst_device_id, p->ttl);
    return ring_link_lowlevel_transmit_payload(p);
}