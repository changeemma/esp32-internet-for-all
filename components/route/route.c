#include <stdio.h>
#include "route.h"

const char *ROUTE_TAG = "==> route";
unsigned num_routes = 1;



esp_ip4_addr_t get_ip_gateway(gateway_t gw){
    esp_ip4_addr_t ip_addr;
    if(gw==SPI_GATEWAY){
        uint32_t addr = get_ring_link_gateway_v4_by_orientation();
        ip_addr.addr = addr;
        return ip_addr;
    }
    esp_err_t err = get_wifi_ip_interface_address();
    if (err != ESP_OK) {
        printf("Failed to convert IP string to struct, error: %d\n", err);
        ip_addr.addr = 0;
    }
    return ip_addr;
}

esp_err_t add_route(char * ip, gateway_t gw, char * netmask, char * key)
{
    esp_netif_t *new_route;
    struct netif *new_route_lwip_netif;
    esp_ip4_addr_t ip_addr;
    esp_ip4_addr_t mask_addr;
    esp_ip4_addr_t gw_addr;
    gw_addr = get_ip_gateway(gw);
    esp_netif_str_to_ip4(ip, &ip_addr);
    esp_netif_str_to_ip4(netmask, &mask_addr);

    const esp_netif_netstack_config_t s_spi_netif_config = {
        .lwip = {
            .init_fn = wlanif_init_sta,
            .input_fn = wlanif_input
        }};
    
    const esp_netif_ip_info_t ip_info = {
        .ip = ip_addr,
        .gw = gw_addr,
        .netmask = mask_addr,
    };
    esp_netif_inherent_config_t base_netif_config = {
        .flags = NETIF_FLAG_BROADCAST | NETIF_FLAG_LINK_UP | ESP_NETIF_FLAG_AUTOUP,
        .ip_info = &ip_info,
        .get_ip_event = 0,
        .lost_ip_event = 0,
        .if_key = key,
        .if_desc = key,
        .route_prio = 15,
        .bridge_info = NULL};

    esp_netif_config_t netif_config = {
        .base = &base_netif_config,
        .stack = &s_spi_netif_config,
        .driver = NULL};
    new_route = esp_netif_new(&netif_config);

    if (new_route == NULL) {
        ESP_LOGE(ROUTE_TAG, "esp_netif_new failed!");
    }
    uint8_t mac[6] = {0x11, 0x11, 0x22, 0x33, 0x44, 0x01};
    mac[5] = num_routes & 0xFF;
    printf("MAC Address: %02X:%02X:%02X:%02X:%02X:%02X\n",
           mac[0], mac[1], mac[2],
           mac[3], mac[4], mac[5]);
    esp_netif_set_mac(new_route, mac);
    new_route_lwip_netif = esp_netif_get_netif_impl(new_route);
    if(!new_route_lwip_netif){
        printf("ES NULL esp_netif_get_netif_impl \n");
    }
    ESP_ERROR_CHECK(esp_netif_attach_wifi_station(new_route));
    esp_netif_action_start(new_route, RING_LINK_TX_EVENT, RING_LINK_EVENT_NEW_ROUTE, NULL);
    num_routes = num_routes + 1;
    return ESP_OK;
}

esp_err_t rm_route(char * ip, char * netmask){
    struct netif *netif;
    esp_netif_t *route_netif;
    esp_ip4_addr_t ip_addr;
    esp_ip4_addr_t mask_addr;
    esp_err_t status = ESP_OK;

    esp_netif_str_to_ip4(ip, &ip_addr);
    esp_netif_str_to_ip4(netmask, &mask_addr);
    for (netif = netif_list; netif != NULL; netif = netif->next) {
        if(ip4_addr_cmp(&ip_addr, netif_ip4_addr(netif)) &&
        ip4_addr_cmp(&mask_addr, netif_ip4_netmask(netif))){
            route_netif = esp_netif_get_handle_from_netif_impl(netif);
            if(!route_netif){
                printf("Error\n");
                status = ESP_FAIL;
            }
            netif_set_down(netif);
            netif_remove(netif);
            esp_netif_destroy(route_netif);
            num_routes = num_routes -1;
        }
    }
    return status;
}

