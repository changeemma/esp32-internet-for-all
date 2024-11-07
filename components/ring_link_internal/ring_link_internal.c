#include "ring_link_internal.h"

static const char* TAG = "==> ring_link_internal";

esp_err_t ring_link_internal_init( void )
{
    return ESP_OK;
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

esp_err_t ring_link_process(ring_link_payload_t *p)
{
    ESP_LOGW(TAG, "call on_sibling_message(%s, %i)\n", p->buffer, p->len);
    return ESP_OK;
}
