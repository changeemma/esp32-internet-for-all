#include "spi_tx_netif.h"

const char *SPI_NETIF_TX_TAG = "==> spi_tx_netif";

u32_t spi_ipv6_addr[6] = {0xfe800000, 0x00000000, 0xb2a1a2ff, 0xfea3b5b6};
uint8_t mac[6] = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55};

ESP_EVENT_DEFINE_BASE(SPI_TX_EVENT);

esp_netif_t *spi_tx_netif;

esp_netif_t * get_spi_tx_netif(void){
    return spi_tx_netif;
}

err_t output_function(struct netif *netif, struct pbuf *p, const ip4_addr_t *ipaddr)
{
    ESP_LOGI(SPI_NETIF_TX_TAG, "Calling output_function(struct netif *netif, struct pbuf *p, const ip4_addr_t *ipaddr)");
    printf("len %u\n", p->len);
    printf("total len %u\n", p->tot_len);
    struct ip_hdr *iphdr = (struct ip_hdr *)p->payload;
    if((IPH_V(iphdr) == 4) || (IPH_V(iphdr) == 6)){
        netif->linkoutput(netif, p);
    }
    return ESP_OK;
}

err_t linkoutput_function(struct netif *netif, struct pbuf *p)
{
    ESP_LOGI(SPI_NETIF_TX_TAG, "Calling linkoutput_function(struct netif *netif, struct pbuf *p)");

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

esp_err_t esp_netif_spi_driver_transmit(void *h, void *buffer, size_t len)
{
    ESP_LOGI(SPI_NETIF_TX_TAG, "spi_netif_driver_transmit(void *h, void *buffer, size_t len) called");
    
    if (buffer==NULL) {
        ESP_LOGI(SPI_NETIF_TX_TAG, "buffer is null");
        return ESP_OK;
    }
    spi_transmit(buffer, len);
    return ESP_OK;
}

err_t spi_netif_netstack_init_fn(struct netif *netif)
{
    LWIP_ASSERT("netif != NULL", (netif != NULL));
    /* Have to get the esp-netif handle from netif first and then driver==ethernet handle from there */
    netif->name[0]= 's';
    netif->name[1] = 'p';
    netif->output = output_function;
    netif->linkoutput = linkoutput_function;
    netif->mtu=1500;
    netif->hwaddr_len = ETH_HWADDR_LEN;
    return ERR_OK;
}

esp_netif_recv_ret_t spi_netif_netstack_input_fn(void *h, void *buffer, size_t len, void* l2_buff)
{
    struct netif *netif = h;
    esp_netif_t *esp_netif = esp_netif_get_handle_from_netif_impl(netif);
    struct pbuf *p;
    printf("spi_netif_netstack_input_fn\n");

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
        LWIP_DEBUGF(NETIF_DEBUG, ("spi_netif_netstack_input_fn: IP input error\n"));
        printf('spi_netif_netstack_input_fn: pbuf_free(p)');
        // pbuf_free(p);
        return ESP_NETIF_OPTIONAL_RETURN_CODE(ESP_FAIL);
    }
    /* the pbuf will be free in upper layer, eg: ethernet_input */
    return ESP_NETIF_OPTIONAL_RETURN_CODE(ESP_OK);
}

esp_err_t esp_netif_spi_driver_transmit_wrap(void *h, void *buffer, size_t len, void *netstack_buffer)
{
    printf("transmit_wrap_function\n");
    return ESP_OK;
};

esp_err_t spi_tx_driver_post_attach(esp_netif_t * esp_netif, void * args)
{
    ESP_LOGI(SPI_NETIF_TX_TAG, "Calling esp_netif_spi_post_attach_start(esp_netif_t * esp_netif, void *args)");
    spi_netif_driver_t driver = (spi_netif_driver_t) args;
    driver->base.netif = esp_netif;
    esp_netif_driver_ifconfig_t driver_ifconfig = {
        .handle = driver,
        .transmit = esp_netif_spi_driver_transmit,
        .transmit_wrap = esp_netif_spi_driver_transmit_wrap,
        .driver_free_rx_buffer = NULL,
    };

    ESP_ERROR_CHECK(esp_netif_set_driver_config(esp_netif, &driver_ifconfig));

    return ESP_OK;
}

void spi_default_handler(void *arg, esp_event_base_t base, int32_t event_id, void *data)
{
    printf("spi_default_handler\n");
    esp_netif_action_got_ip(spi_tx_netif, base, event_id, data);
}


void spi_default_action_sta_start(void *arg, esp_event_base_t base, int32_t event_id, void *data)
{
    printf("spi_default_action_sta_start\n");
    esp_netif_action_start(spi_tx_netif, base, event_id, data);
    const esp_ip_addr_t spi_ip6_addr = ESP_IP6ADDR_INIT(spi_ipv6_addr[0], spi_ipv6_addr[1], spi_ipv6_addr[2], spi_ipv6_addr[3]);
    esp_netif_set_ip6_linklocal(spi_tx_netif, spi_ip6_addr);
    esp_netif_set_default_netif(spi_tx_netif);
}

esp_err_t spi_tx_netif_init(void)
{
    const esp_netif_netstack_config_t s_spi_netif_config = {
        .lwip = {
            .init_fn = spi_netif_netstack_init_fn,
            .input_fn = spi_netif_netstack_input_fn
        }};
    const esp_netif_ip_info_t ip_info = {
        .ip = {.addr = get_spi_ip_v4_by_orientation()},
        .gw = {.addr = get_spi_gateway_v4_by_orientation()},
        .netmask = {.addr = ESP_IP4TOADDR(0, 0, 0, 0)},
    };
    esp_netif_inherent_config_t base_netif_config = {
        .flags = NETIF_FLAG_BROADCAST | NETIF_FLAG_LINK_UP | ESP_NETIF_FLAG_AUTOUP,
        .ip_info = &ip_info,
        .get_ip_event = 0,
        .lost_ip_event = 0,
        .if_key = "spi_tx",
        .if_desc = "spi-tx if",
        .route_prio = 15,
        .bridge_info = NULL};

    esp_netif_config_t netif_config = {
        .base = &base_netif_config,
        .stack = &s_spi_netif_config,
        .driver = NULL};
    
    spi_netif_init(&spi_tx_netif, &netif_config, spi_tx_driver_post_attach);

    ESP_ERROR_CHECK(esp_event_handler_instance_register(SPI_TX_EVENT, SPI_EVENT_START, spi_default_action_sta_start, NULL, NULL));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, spi_default_handler, NULL, NULL));
    ESP_ERROR_CHECK(esp_event_post(SPI_TX_EVENT, SPI_EVENT_START, NULL, 0, portMAX_DELAY));
    return ESP_OK;
}
