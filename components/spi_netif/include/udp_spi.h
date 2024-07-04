#pragma once

#include "lwip/err.h"
#include "lwip/udp.h"
#include "lwip/sys.h"
#include "esp_netif.h"
#include "esp_netif_net_stack.h"
#include "esp_log.h"

#include "spi_netif.h"


#ifdef __cplusplus
extern "C" {
#endif

void bind_udp_spi();
u16_t get_active_spi_port(void);
u16_t get_active_spi_port_ipv6(void);
typedef void (*udp_recv_fn)(void *arg, struct udp_pcb *pcb, struct pbuf *p, const ip_addr_t *addr, u16_t port);
void recv_upd_data(void *arg, struct udp_pcb *pcb, struct pbuf *p, const ip_addr_t *addr, u16_t port);
void bind_udp_to_netif(esp_netif_t *esp_netif, u16_t port);


#ifdef __cplusplus
}
#endif