#include "spi_payload.h"

static const char* TAG = "==> spi-payload";


bool spi_payload_is_for_device(spi_payload_t *p)
{
    return p->dst_device_id == device_config_get_id() \
        || (p->dst_device_id == DEVICE_ID_ANY);
}

bool spi_payload_is_from_device(spi_payload_t *p)
{
    return p->src_device_id == device_config_get_id();
}

bool spi_payload_is_broadcast(spi_payload_t *p)
{
    return p->dst_device_id == DEVICE_ID_ALL;
}

esp_err_t spi_payload_transmit(spi_payload_t *p)
{
    return spi_lowlevel_transmit(p, sizeof(spi_payload_t));
}

esp_err_t spi_payload_receive(spi_payload_t *p)
{
    return spi_lowlevel_receive(p, sizeof(spi_payload_t));
}

esp_err_t spi_payload_forward(spi_payload_t *p)
{
    if (p->ttl <= 0) {
        ESP_LOGW(TAG, "Discarding packet id '%i' ('%s') due to TTL. Won't forward.", p->id, p->buffer);
        return ESP_OK;
    }
    p->ttl--;
    ESP_LOGI(TAG, "Forwarding packet id '%i' with dest '%i' (TTL=%i).", p->id, p->dst_device_id, p->ttl);
    return spi_payload_transmit(p);
}
