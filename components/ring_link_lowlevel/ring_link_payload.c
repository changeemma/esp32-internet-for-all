#include "ring_link_payload.h"
#include "esp_timer.h"

static const char* TAG = "==> ring_link_payload";


bool ring_link_payload_is_for_device(ring_link_payload_t *p)
{
    bool result;
    int64_t transaction_start_time_ = esp_timer_get_time();
    result = p->dst_id == config_get_id() \
        || (p->dst_id == CONFIG_ID_ANY);
    int64_t end_time = esp_timer_get_time();
    int64_t duration = end_time - transaction_start_time_;
    ESP_LOGW(TAG, "Mensaje ring_link_payload_is_for_device: en %lld μs", duration);
    return result;
}

bool ring_link_payload_is_from_device(ring_link_payload_t *p)
{
    return p->src_id == config_get_id();
}

bool ring_link_payload_is_broadcast(ring_link_payload_t *p)
{
    return p->dst_id == CONFIG_ID_ALL;
}

bool ring_link_payload_is_internal(ring_link_payload_t *p)
{
    return p->buffer_type == RING_LINK_PAYLOAD_TYPE_INTERNAL;
}

bool ring_link_payload_is_esp_netif(ring_link_payload_t *p)
{
    return p->buffer_type == RING_LINK_PAYLOAD_TYPE_ESP_NETIF;
}
