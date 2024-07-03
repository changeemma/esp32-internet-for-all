#include "lwip/netif.h"

#undef LWIP_HOOK_IP4_ROUTE_SRC
#define LWIP_HOOK_IP4_ROUTE_SRC custom_ip4_route_src_hook

struct netif* custom_ip4_route_src_hook(const ip4_addr_t *src, const ip4_addr_t *dest);