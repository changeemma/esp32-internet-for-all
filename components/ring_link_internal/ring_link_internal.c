#include "ring_link_internal.h"

static const char* TAG = "==> ring_link_internal";

esp_err_t ring_link_internal_init( void )
{
    return ESP_OK;
}

esp_err_t ring_link_process(ring_link_payload_t *p)
{
    ESP_LOGW(TAG, "call on_sibling_message(%s, %i)\n", p->buffer, p->len);
    return ESP_OK;
}
