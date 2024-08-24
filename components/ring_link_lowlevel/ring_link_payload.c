#include "ring_link_payload.h"

static const char* TAG = "==> ring_link_payload";


bool ring_link_payload_is_for_device(ring_link_payload_t *p)
{
    return p->dst_device_id == device_config_get_id() \
        || (p->dst_device_id == DEVICE_ID_ANY);
}

bool ring_link_payload_is_from_device(ring_link_payload_t *p)
{
    return p->src_device_id == device_config_get_id();
}

bool ring_link_payload_is_broadcast(ring_link_payload_t *p)
{
    return p->dst_device_id == DEVICE_ID_ALL;
}

bool ring_link_payload_is_heartbeat(ring_link_payload_t *p)
{
    return IS_HEARTBEAT_PAYLOAD(p->buffer_type);
}
