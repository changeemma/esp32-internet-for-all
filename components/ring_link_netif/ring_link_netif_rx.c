#include "ring_link_netif_rx.h"

static const char* TAG = "==> ring_link_netif_rx";

ESP_EVENT_DEFINE_BASE(RING_LINK_RX_EVENT);

static esp_netif_t *ring_link_rx_netif = NULL;


esp_err_t ring_link_rx_netif_receive(ring_link_payload_t *p)
{
    printf("call esp_netif_receive(netif, %s, %i, NULL)\n", p->buffer, p->len);
    struct pbuf *q;
    q = pbuf_alloc(PBUF_TRANSPORT, 500, PBUF_POOL);

    ESP_LOGI(TAG, "esp_netif_ring_link_receive_task()...");
    if (p->len <= 0) {
        printf("an error ocurred receiving or empty data\n");
        return ESP_OK;
    }
    printf("%d\n", p->len);
    ESP_LOGI(TAG, "Calling esp_netif_receive(netif, ring_link_rx_buffer, len, NULL)");
    q->payload = p->buffer;
    struct ip_hdr *iphdr = (struct ip_hdr *)q->payload;
    if((IPH_V(iphdr) == 4) || (IPH_V(iphdr) == 6)){
        q->next = NULL;
        q->len = lwip_ntohs(IPH_LEN(iphdr));
        q->tot_len = p->len/8;
        esp_netif_receive(ring_link_rx_netif, q, q->tot_len, NULL);
    }
    return ESP_OK;
}

static err_t ring_link_rx_netstack_lwip_init_fn(struct netif *netif)
{
    LWIP_ASSERT("netif != NULL", (netif != NULL));
    /* Have to get the esp-netif handle from netif first and then driver==ethernet handle from there */
    netif->name[0]= 'r';
    netif->name[1] = 'x';
    netif->mtu = RING_LINK_NETIF_MTU;
    netif->hwaddr_len = ETH_HWADDR_LEN;
    return ERR_OK;
}

static esp_netif_recv_ret_t ring_link_rx_netstack_lwip_input_fn(void *h, void *buffer, size_t len, void* l2_buff)
{
    struct netif *netif = h;
    esp_netif_t *esp_netif = esp_netif_get_handle_from_netif_impl(netif);
    struct pbuf *p;
    printf("ring_link_netif_rx_netstack_input_fn\n");

    /* allocate custom pbuf to hold  */
    p = pbuf_alloc(PBUF_TRANSPORT, 120, PBUF_POOL);
    pbuf_copy(p, buffer);

    if (p == NULL || netif == NULL) {
        printf('Entra aca esp_netif_free_rx_buffer\n');
        // esp_netif_free_rx_buffer(esp_netif, buffer);
        return ESP_NETIF_OPTIONAL_RETURN_CODE(ESP_ERR_NO_MEM);
    }
    /* full packet send to tcpip_thread to process */
    if (unlikely(netif->input(p, netif) != ERR_OK)) {
        LWIP_DEBUGF(NETIF_DEBUG, ("ring_link_netif_rx_netstack_input_fn: IP input error\n"));
        printf('ring_link_netif_rx_netstack_input_fn: pbuf_free(p)');
        // pbuf_free(p);
        return ESP_NETIF_OPTIONAL_RETURN_CODE(ESP_FAIL);
    }
    /* the pbuf will be free in upper layer, eg: ethernet_input */
    return ESP_NETIF_OPTIONAL_RETURN_CODE(ESP_OK);
}

static esp_err_t ring_link_rx_driver_transmit(void *h, void *buffer, size_t len)
{
    ESP_LOGW(TAG, "ring_link_rx_driver_transmit(void *h, void *buffer, size_t len) called");
    
    if (buffer==NULL) {
        ESP_LOGW(TAG, "buffer is null");
        return ESP_OK;
    }
    return ESP_OK;
}

static esp_err_t ring_link_rx_driver_post_attach(esp_netif_t * esp_netif, void * args)
{
    ESP_LOGI(TAG, "Calling esp_netif_ring_link_post_attach_start(esp_netif_t * esp_netif, void *args)");
    ring_link_netif_driver_t driver = (ring_link_netif_driver_t) args;
    driver->base.netif = esp_netif;
    esp_netif_driver_ifconfig_t driver_ifconfig = {
        .handle = driver,
        .transmit = ring_link_rx_driver_transmit,
        .driver_free_rx_buffer = NULL,
    };

    ESP_ERROR_CHECK(esp_netif_set_driver_config(esp_netif, &driver_ifconfig));
    return ESP_OK;
}

static void ring_link_rx_default_handler(void *arg, esp_event_base_t base, int32_t event_id, void *data)
{
    printf("ring_link_default_handler\n");
    esp_netif_action_got_ip(ring_link_rx_netif, base, event_id, data);
}

static void ring_link_rx_default_action_sta_start(void *arg, esp_event_base_t base, int32_t event_id, void *data)
{
    printf("ring_link_default_action_sta_start\n");
    u32_t ring_link_ipv6_addr[6] = {0xfe800000, 0x00000000, 0xb2a1a2ff, 0xfea3b5b6};
    const esp_ip_addr_t ring_link_ip6_addr = ESP_IP6ADDR_INIT(ring_link_ipv6_addr[0], ring_link_ipv6_addr[1], ring_link_ipv6_addr[2], ring_link_ipv6_addr[3]);

    esp_netif_action_start(ring_link_rx_netif, base, event_id, data);
    esp_netif_set_ip6_linklocal(ring_link_rx_netif, ring_link_ip6_addr);
}

esp_err_t ring_link_rx_netif_init(void)
{
    printf("ring_link_rx_netif_init\n");
    const esp_netif_netstack_config_t s_ring_link_netif_config = {
        .lwip = {
            .init_fn = ring_link_rx_netstack_lwip_init_fn,
            .input_fn = ring_link_rx_netstack_lwip_input_fn
        }};
    const esp_netif_ip_info_t ip_info = {
        .ip = {.addr = ESP_IP4TOADDR(127, 0, 0, 13)},
        .gw = {.addr = ESP_IP4TOADDR(127, 0, 0, 13)},
        .netmask = {.addr = ESP_IP4TOADDR(0, 0, 0, 0)},
    };
    esp_netif_inherent_config_t base_netif_config = {
        .flags = NETIF_FLAG_BROADCAST | NETIF_FLAG_LINK_UP | ESP_NETIF_FLAG_AUTOUP,
        .ip_info = &ip_info,
        .get_ip_event = 0,
        .lost_ip_event = 0,
        .if_key = "ring_link_rx",
        .if_desc = "ring-link-rx if",
        .route_prio = 15,
        .bridge_info = NULL};

    esp_netif_config_t netif_config = {
        .base = &base_netif_config,
        .stack = &s_ring_link_netif_config,
        .driver = NULL};
    
    ring_link_netif_esp_netif_init(&ring_link_rx_netif, &netif_config, ring_link_rx_driver_post_attach);

    uint8_t mac[6] = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55};
    esp_netif_set_mac(ring_link_rx_netif, mac);
    
    ESP_ERROR_CHECK(esp_event_handler_instance_register(RING_LINK_RX_EVENT, RING_LINK_EVENT_START, ring_link_rx_default_action_sta_start, NULL, NULL));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, ring_link_rx_default_handler, NULL, NULL));
    ESP_ERROR_CHECK(esp_event_post(RING_LINK_RX_EVENT, RING_LINK_EVENT_START, NULL, 0, portMAX_DELAY));
    return ESP_OK;
}
