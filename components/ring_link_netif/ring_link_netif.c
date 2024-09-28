#include "ring_link_netif.h"

static const char* TAG = "==> ring_link_netif";


esp_err_t ring_link_netif_handler(ring_link_payload_t *p)
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

esp_err_t ring_link_netif_init(void)
{
    ESP_ERROR_CHECK(ring_link_rx_netif_init());
    ESP_ERROR_CHECK(ring_link_tx_netif_init());
    return ESP_OK;
}
