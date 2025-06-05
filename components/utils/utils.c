#include "utils.h"

const char *UTILS_TAG = "==> utils";

void print_route_table(void)
{
    struct netif *netif;
    printf("Listing network interfaces...\n");
    // for (netif = netif_list; netif != NULL; netif = netif->next)
    // {
    //     printf("------------------------\n");
    //     printf("IPv4 route table for %s:\n", netif->name);
    //     printf("ip4: %s\n", ip4addr_ntoa(&netif->ip_addr.u_addr));
    //     printf("netmask: %s\n", ip4addr_ntoa(&netif->netmask.u_addr));
    //     printf("gw: %s\n", ip4addr_ntoa(&netif->gw.u_addr));
    //     printf("ip6 linklocal: %s\n", ip6addr_ntoa(&netif->ip6_addr->u_addr.ip6) );
    // }
    // printf("------------------------\n");
}
