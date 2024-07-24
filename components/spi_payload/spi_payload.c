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
