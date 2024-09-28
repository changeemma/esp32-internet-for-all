#include "ring_link_netif_tx.h"

static const char* TAG = "==> ring_link_netif";


ESP_EVENT_DEFINE_BASE(RING_LINK_TX_EVENT);


static esp_netif_t *ring_link_tx_netif = NULL;


esp_netif_t *get_ring_link_tx_netif(void){
    return ring_link_tx_netif;
}


static err_t output_function(struct netif *netif, struct pbuf *p, const ip4_addr_t *ipaddr)
{
    ESP_LOGI(TAG, "Calling output_function(struct netif *netif, struct pbuf *p, const ip4_addr_t *ipaddr)");
    printf("len %u\n", p->len);
    printf("total len %u\n", p->tot_len);
    struct ip_hdr *iphdr = (struct ip_hdr *)p->payload;
    if((IPH_V(iphdr) == 4) || (IPH_V(iphdr) == 6)){
        netif->linkoutput(netif, p);
    }
    return ESP_OK;
}

static err_t linkoutput_function(struct netif *netif, struct pbuf *p)
{
    ESP_LOGI(TAG, "Calling linkoutput_function(struct netif *netif, struct pbuf *p)");

    struct pbuf *q = p;
    u16_t alloc_len = (u16_t)(p->tot_len);
    ip4_debug_print(q);
    esp_netif_t *esp_netif = esp_netif_get_handle_from_netif_impl(netif);
    esp_err_t ret = ESP_FAIL;

    if (!esp_netif) {
        LWIP_DEBUGF(NETIF_DEBUG, ("corresponding esp-netif is NULL: netif=%p pbuf=%p len=%d\n", netif, p, p->len));
        return ERR_IF;
    }
    if (q->next == NULL) {
        ret = esp_netif_transmit(esp_netif, q->payload, q->len);
    } else {
        LWIP_DEBUGF(PBUF_DEBUG, ("low_level_output: pbuf is a list, application may has bug"));
        q = pbuf_alloc(PBUF_RAW_TX, p->tot_len, PBUF_RAM);
        if (q != NULL) {
            pbuf_copy(q, p);
        } else {
            return ERR_MEM;
        }
        ret = esp_netif_transmit(esp_netif, q->payload, q->len);
        /* content in payload has been copied to DMA buffer, it's safe to free pbuf now */
        pbuf_free(q);
    }
    /* Check error */
    if (likely(ret == ESP_OK)) {
        return ERR_OK;
    }
    if (ret == ESP_ERR_NO_MEM) {
        return ERR_MEM;
    }
    return ERR_IF;

}

static esp_err_t esp_netif_ring_link_driver_transmit(void *h, void *buffer, size_t len)
{
    ESP_LOGI(TAG, "ring_link_netif_driver_transmit(void *h, void *buffer, size_t len) called");
    
    if (buffer==NULL) {
        ESP_LOGI(TAG, "buffer is null");
        return ESP_OK;
    }
    ring_link_payload_t p = {
        .id = 0,
        .ttl = RING_LINK_PAYLOAD_TTL,
        .src_device_id = device_config_get_id(),
        .dst_device_id = DEVICE_ID_ANY,
        .buffer_type = RING_LINK_PAYLOAD_TYPE_ESP_NETIF,
        .len = len,
    };
    memccpy(p.buffer, buffer, len, RING_LINK_PAYLOAD_BUFFER_SIZE);
    return ring_link_lowlevel_transmit_payload(&p);
}

static err_t ring_link_tx_netif_netstack_init_fn(struct netif *netif)
{
    LWIP_ASSERT("netif != NULL", (netif != NULL));
    /* Have to get the esp-netif handle from netif first and then driver==ethernet handle from there */
    netif->name[0]= 's';
    netif->name[1] = 'p';
    netif->output = output_function;
    netif->linkoutput = linkoutput_function;
    netif->mtu = RING_LINK_NETIF_MTU;
    netif->hwaddr_len = ETH_HWADDR_LEN;
    return ERR_OK;
}

static esp_netif_recv_ret_t ring_link_tx_netif_netstack_input_fn(void *h, void *buffer, size_t len, void* l2_buff)
{
    struct netif *netif = h;
    esp_netif_t *esp_netif = esp_netif_get_handle_from_netif_impl(netif);
    struct pbuf *p;
    printf("ring_link_netif_netstack_input_fn\n");

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
        LWIP_DEBUGF(NETIF_DEBUG, ("ring_link_netif_netstack_input_fn: IP input error\n"));
        printf('ring_link_netif_netstack_input_fn: pbuf_free(p)');
        // pbuf_free(p);
        return ESP_NETIF_OPTIONAL_RETURN_CODE(ESP_FAIL);
    }
    /* the pbuf will be free in upper layer, eg: ethernet_input */
    return ESP_NETIF_OPTIONAL_RETURN_CODE(ESP_OK);
}

static esp_err_t esp_netif_ring_link_driver_transmit_wrap(void *h, void *buffer, size_t len, void *netstack_buffer)
{
    printf("transmit_wrap_function\n");
    return ESP_OK;
};

static esp_err_t ring_link_tx_driver_post_attach(esp_netif_t * esp_netif, void * args)
{
    ESP_LOGI(TAG, "Calling esp_netif_ring_link_post_attach_start(esp_netif_t * esp_netif, void *args)");
    ring_link_netif_driver_t driver = (ring_link_netif_driver_t) args;
    driver->base.netif = esp_netif;
    esp_netif_driver_ifconfig_t driver_ifconfig = {
        .handle = driver,
        .transmit = esp_netif_ring_link_driver_transmit,
        .transmit_wrap = esp_netif_ring_link_driver_transmit_wrap,
        .driver_free_rx_buffer = NULL,
    };

    ESP_ERROR_CHECK(esp_netif_set_driver_config(esp_netif, &driver_ifconfig));

    return ESP_OK;
}

static void ring_link_tx_default_handler(void *arg, esp_event_base_t base, int32_t event_id, void *data)
{
    ESP_LOGI(TAG, "Calling ring_link_tx_default_handler");
    esp_netif_action_got_ip(ring_link_tx_netif, base, event_id, data);
}


static void ring_link_tx_default_action_start(void *arg, esp_event_base_t base, int32_t event_id, void *data)
{
    ESP_LOGI(TAG, "Calling ring_link_tx_default_action_start");
    u32_t ring_link_ipv6_addr[6] = {0xfe800000, 0x00000000, 0xb2a1a2ff, 0xfea3b5b6};
    const esp_ip_addr_t ring_link_ip6_addr = ESP_IP6ADDR_INIT(ring_link_ipv6_addr[0], ring_link_ipv6_addr[1], ring_link_ipv6_addr[2], ring_link_ipv6_addr[3]);

    esp_netif_action_start(ring_link_tx_netif, base, event_id, data);
    esp_netif_set_ip6_linklocal(ring_link_tx_netif, ring_link_ip6_addr);
    esp_netif_set_default_netif(ring_link_tx_netif);
}

esp_err_t ring_link_tx_netif_init(void)
{
    ESP_LOGI(TAG, "Calling ring_link_tx_netif_init");
    const esp_netif_netstack_config_t s_ring_link_netif_config = {
        .lwip = {
            .init_fn = ring_link_tx_netif_netstack_init_fn,
            .input_fn = ring_link_tx_netif_netstack_input_fn
        }};
    const esp_netif_ip_info_t ip_info = {
        .ip = {.addr = get_ring_link_ip_v4_by_orientation()},
        .gw = {.addr = get_ring_link_gateway_v4_by_orientation()},
        .netmask = {.addr = ESP_IP4TOADDR(0, 0, 0, 0)},
    };
    esp_netif_inherent_config_t base_netif_config = {
        .flags = NETIF_FLAG_BROADCAST | NETIF_FLAG_LINK_UP | ESP_NETIF_FLAG_AUTOUP,
        .ip_info = &ip_info,
        .get_ip_event = 0,
        .lost_ip_event = 0,
        .if_key = "ring_link_tx",
        .if_desc = "ring-link-tx if",
        .route_prio = 15,
        .bridge_info = NULL};

    esp_netif_config_t netif_config = {
        .base = &base_netif_config,
        .stack = &s_ring_link_netif_config,
        .driver = NULL};
    

    ring_link_tx_netif = esp_netif_new(&netif_config);

    if (ring_link_tx_netif == NULL) {
        ESP_LOGE(TAG, "esp_netif_new failed!");
    }
    
    ring_link_netif_esp_netif_attach(ring_link_tx_netif, ring_link_tx_driver_post_attach);
    
    uint8_t mac[6] = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55};
    esp_netif_set_mac(ring_link_tx_netif, mac);

    ESP_ERROR_CHECK(esp_event_handler_instance_register(RING_LINK_TX_EVENT, RING_LINK_EVENT_START, ring_link_tx_default_action_start, NULL, NULL));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, ring_link_tx_default_handler, NULL, NULL));
    ESP_ERROR_CHECK(esp_event_post(RING_LINK_TX_EVENT, RING_LINK_EVENT_START, NULL, 0, portMAX_DELAY));
    return ESP_OK;
}
