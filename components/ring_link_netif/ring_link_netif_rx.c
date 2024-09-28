#include "ring_link_netif_rx.h"

static const char* TAG = "==> ring_link_netif_rx";

ESP_EVENT_DEFINE_BASE(RING_LINK_RX_EVENT);

static esp_netif_t *ring_link_rx_netif = NULL;

static const struct esp_netif_netstack_config netif_netstack_config = {
    .lwip = {
        .init_fn = ring_link_rx_netstack_lwip_init_fn,
        .input_fn = ring_link_rx_netstack_lwip_input_fn
    }
};

static const esp_netif_inherent_config_t netif_inherent_config = {
    .flags = (esp_netif_flags_t)(NETIF_FLAG_BROADCAST | NETIF_FLAG_LINK_UP | ESP_NETIF_FLAG_AUTOUP),
    ESP_COMPILER_DESIGNATED_INIT_AGGREGATE_TYPE_EMPTY(mac)
    ESP_COMPILER_DESIGNATED_INIT_AGGREGATE_TYPE_EMPTY(ip_info)
    .get_ip_event = 0,
    .lost_ip_event = 0,
    .if_key = "ring_link_rx",
    .if_desc = "ring-link-rx if",
    .route_prio = 15,
    .bridge_info = NULL
};

static const esp_netif_config_t netif_config = {
    .base = &netif_inherent_config,
    .driver = NULL,
    .stack = &netif_netstack_config,
};

esp_err_t ring_link_rx_netif_receive(ring_link_payload_t *p)
{
    struct pbuf *q;
    struct ip_hdr *ip_header;

    if (p->len <= 0) {
        ESP_LOGW(TAG, "Discarding empty payload.");
        return ESP_OK;
    }
    ip_header = (struct ip_hdr *) p->buffer;
    if (!((IPH_V(ip_header) == 4) || (IPH_V(ip_header) == 6)))
    {
        ESP_LOGW(TAG, "Discarding non-IP payload.");
        return ESP_OK;
    }

    q = pbuf_alloc(PBUF_TRANSPORT, RING_LINK_NETIF_MTU, PBUF_POOL);
    q->payload = p->buffer;

    q->next = NULL;
    q->len = lwip_ntohs(IPH_LEN((struct ip_hdr *) p->buffer));
    q->tot_len = p->len/8;
    
    return esp_netif_receive(ring_link_rx_netif, q, q->tot_len, NULL);
}

err_t ring_link_rx_netstack_lwip_init_fn(struct netif *netif)
{
    LWIP_ASSERT("netif != NULL", (netif != NULL));
    /* Have to get the esp-netif handle from netif first and then driver==ethernet handle from there */
    netif->name[0]= 'r';
    netif->name[1] = 'x';
    netif->mtu = RING_LINK_NETIF_MTU;
    netif->hwaddr_len = ETH_HWADDR_LEN;
    return ERR_OK;
}

esp_netif_recv_ret_t ring_link_rx_netstack_lwip_input_fn(void *h, void *buffer, size_t len, void* l2_buff)
{
    struct netif *netif = h;
    struct pbuf *p;
    printf("ring_link_netif_rx_netstack_input_fn\n");

    /* allocate custom pbuf to hold  */
    p = pbuf_alloc(PBUF_TRANSPORT, RING_LINK_NETIF_MTU, PBUF_POOL);
    pbuf_copy(p, buffer);

    if (p == NULL || netif == NULL) {
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
    ESP_LOGW(TAG, "This netif should not transmit but ring_link_rx_driver_transmit was called");
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
    ESP_LOGI(TAG, "Calling ring_link_rx_default_handler");
    esp_netif_action_got_ip(ring_link_rx_netif, base, event_id, data);
}

static void ring_link_rx_default_action_start(void *arg, esp_event_base_t base, int32_t event_id, void *data)
{
    ESP_LOGI(TAG, "Calling ring_link_rx_default_action_start");
    u32_t ring_link_ipv6_addr[6] = {0xfe800000, 0x00000000, 0xb2a1a2ff, 0xfea3b5b6};
    const esp_ip_addr_t ring_link_ip6_addr = ESP_IP6ADDR_INIT(ring_link_ipv6_addr[0], ring_link_ipv6_addr[1], ring_link_ipv6_addr[2], ring_link_ipv6_addr[3]);


    esp_netif_set_ip6_linklocal(ring_link_rx_netif, ring_link_ip6_addr);

    const esp_netif_ip_info_t ip_info = {
        .ip = {.addr = ESP_IP4TOADDR(127, 0, 0, 13)},
        .gw = {.addr = ESP_IP4TOADDR(127, 0, 0, 13)},
        .netmask = {.addr = ESP_IP4TOADDR(0, 0, 0, 0)},
    };
    
    ESP_ERROR_CHECK(esp_netif_dhcps_stop(ring_link_rx_netif));
    ESP_ERROR_CHECK(esp_netif_set_ip_info(ring_link_rx_netif, &ip_info));
    //ESP_ERROR_CHECK(esp_netif_dhcps_start(ring_link_rx_netif));

    uint8_t mac[6] = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55};
    esp_netif_set_mac(ring_link_rx_netif, mac);
    esp_netif_action_start(ring_link_rx_netif, base, event_id, data);
}


esp_err_t ring_link_rx_netif_init(void)
{
    ESP_LOGI(TAG, "Calling ring_link_rx_netif_init");
    ring_link_rx_netif = esp_netif_new(&netif_config);

    if (ring_link_rx_netif == NULL) {
        ESP_LOGE(TAG, "esp_netif_new failed!");
    }

    ring_link_netif_esp_netif_attach(ring_link_rx_netif, ring_link_rx_driver_post_attach);

    ESP_ERROR_CHECK(esp_event_handler_instance_register(RING_LINK_RX_EVENT, RING_LINK_EVENT_START, ring_link_rx_default_action_start, NULL, NULL));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, ring_link_rx_default_handler, NULL, NULL));
    ESP_ERROR_CHECK(esp_event_post(RING_LINK_RX_EVENT, RING_LINK_EVENT_START, NULL, 0, portMAX_DELAY));
    return ESP_OK;
}
