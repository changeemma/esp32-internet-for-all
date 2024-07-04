#include "spi_netif.h"

static const char* TAG = "==> spi-netif";

u32_t spi_ipv6_addr[6] = {0xfe800000, 0x00000000, 0xb2a1a2ff, 0xfea3b5b6};
uint8_t mac[6] = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55};

ESP_EVENT_DEFINE_BASE(SPI_TX_EVENT);
ESP_EVENT_DEFINE_BASE(SPI_RX_EVENT);


esp_netif_t *spi_rx_netif = NULL;
esp_netif_t *spi_tx_netif = NULL;

esp_netif_t *get_spi_tx_netif(void){
    return spi_tx_netif;
}


static esp_err_t spi_netif_process(spi_payload_t *p)
{
    printf("call esp_netif_receive(netif, %s, %i, NULL)\n", p->buffer, p->len);
    struct pbuf *q;
    q = pbuf_alloc(PBUF_TRANSPORT, 500, PBUF_POOL);

    ESP_LOGI(TAG, "esp_netif_spi_receive_task()...");
    if (p->len <= 0) {
        printf("an error ocurred receiving or empty data\n");
        return ESP_OK;
    }
    printf("%d\n", p->len);
    ESP_LOGI(TAG, "Calling esp_netif_receive(netif, spi_rx_buffer, len, NULL)");
    q->payload = p->buffer;
    struct ip_hdr *iphdr = (struct ip_hdr *)q->payload;
    if((IPH_V(iphdr) == 4) || (IPH_V(iphdr) == 6)){
        q->next = NULL;
        q->len = lwip_ntohs(IPH_LEN(iphdr));
        q->tot_len = p->len/8;
        esp_netif_receive(spi_rx_netif, q, q->tot_len, NULL);
    }
    return ESP_OK;
}

esp_err_t spi_netif_handler(spi_payload_t *p)
{    
    if (spi_payload_is_from_device(p))  // bounced package
    {
        ESP_LOGW(TAG, "Discarding bounced packet. id '%i' ('%s').", p->id, p->buffer);
        return ESP_OK;
    }
    else if (spi_payload_is_for_device(p))  // payload for me
    {
        return spi_netif_process(p);
    }
    else  // not for me, warn and discard
    {
        ESP_LOGW(TAG, "Discarding packet. id '%i' ('%s').", p->id, p->buffer);
        return ESP_OK;        
    }
}


uint32_t get_spi_ip_v4_by_orientation(void){
    device_orientation_t orientation;
    orientation = device_config_get_orientation();
    return ESP_IP4TOADDR(127, 0, 0, (int) orientation + 1);
}

uint32_t get_spi_gateway_v4_by_orientation(void){
    device_orientation_t orientation;
    orientation = device_config_get_orientation();
    return ESP_IP4TOADDR(127, 0, 0, (int) orientation + 1);
}

static esp_err_t spi_netif_esp_netif_attach(esp_netif_t * esp_netif, esp_err_t (*post_attach_callback)(esp_netif_t *, void *))
{

    spi_netif_driver_t driver = calloc(1, sizeof(struct spi_netif_driver));
    if (driver == NULL)
    {
        ESP_LOGE(TAG, "No memory to create a spi interface handle");
        return ESP_FAIL;
    }

    driver->base.post_attach = post_attach_callback;
    ESP_LOGI(TAG, "esp_spi_create_if_driver() called");
    
    if (driver == NULL)
    {
        ESP_LOGE(TAG, "Failed to create spi interface handle");
        return ESP_FAIL;
    }
    ESP_LOGI(TAG, "esp_netif_attach_spi(esp_netif_t * esp_netif) called");

    return esp_netif_attach(esp_netif, driver);
}



static esp_err_t spi_netif_esp_netif_init(esp_netif_t **esp_netif, esp_netif_config_t *esp_netif_config, esp_err_t (*post_attach_callback)(esp_netif_t *, void *))
{       
    *esp_netif = esp_netif_new(esp_netif_config);

    if (*esp_netif == NULL) {
        ESP_LOGE(TAG, "esp_netif_new failed!");
    }

    struct netif *spi_netif_impl = esp_netif_get_netif_impl(*esp_netif);
    netif_set_flags(spi_netif_impl, NETIF_FLAG_BROADCAST);
    spi_netif_esp_netif_attach(*esp_netif, post_attach_callback);

    return ESP_OK;
}


err_t spi_rx_netstack_lwip_init_fn(struct netif *netif)
{
    LWIP_ASSERT("netif != NULL", (netif != NULL));
    /* Have to get the esp-netif handle from netif first and then driver==ethernet handle from there */
    netif->name[0]= 'r';
    netif->name[1] = 'x';
    netif->mtu=1500;
    netif->hwaddr_len = ETH_HWADDR_LEN;
    return ERR_OK;
}

esp_netif_recv_ret_t spi_rx_netstack_lwip_input_fn(void *h, void *buffer, size_t len, void* l2_buff)
{
    struct netif *netif = h;
    esp_netif_t *esp_netif = esp_netif_get_handle_from_netif_impl(netif);
    struct pbuf *p;
    printf("spi_netif_rx_netstack_input_fn\n");

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
        LWIP_DEBUGF(NETIF_DEBUG, ("spi_netif_rx_netstack_input_fn: IP input error\n"));
        printf('spi_netif_rx_netstack_input_fn: pbuf_free(p)');
        // pbuf_free(p);
        return ESP_NETIF_OPTIONAL_RETURN_CODE(ESP_FAIL);
    }
    /* the pbuf will be free in upper layer, eg: ethernet_input */
    return ESP_NETIF_OPTIONAL_RETURN_CODE(ESP_OK);
}

esp_err_t spi_rx_driver_transmit(void *h, void *buffer, size_t len)
{
    ESP_LOGW(TAG, "spi_rx_driver_transmit(void *h, void *buffer, size_t len) called");
    
    if (buffer==NULL) {
        ESP_LOGW(TAG, "buffer is null");
        return ESP_OK;
    }
    return ESP_OK;
}

esp_err_t spi_rx_driver_post_attach(esp_netif_t * esp_netif, void * args)
{
    ESP_LOGI(TAG, "Calling esp_netif_spi_post_attach_start(esp_netif_t * esp_netif, void *args)");
    spi_netif_driver_t driver = (spi_netif_driver_t) args;
    driver->base.netif = esp_netif;
    esp_netif_driver_ifconfig_t driver_ifconfig = {
        .handle = driver,
        .transmit = spi_rx_driver_transmit,
        .driver_free_rx_buffer = NULL,
    };

    ESP_ERROR_CHECK(esp_netif_set_driver_config(esp_netif, &driver_ifconfig));
    return ESP_OK;
}

void spi_rx_default_handler(void *arg, esp_event_base_t base, int32_t event_id, void *data)
{
    printf("spi_default_handler\n");
    esp_netif_action_got_ip(spi_rx_netif, base, event_id, data);
}

void spi_rx_default_action_sta_start(void *arg, esp_event_base_t base, int32_t event_id, void *data)
{
    printf("spi_default_action_sta_start\n");
    esp_netif_action_start(spi_rx_netif, base, event_id, data);
    //esp_netif_set_default_netif(spi_netif_rx);
}



esp_err_t spi_rx_netif_init(void)
{
    printf("spi_rx_netif_init\n");
    const esp_netif_netstack_config_t s_spi_netif_config = {
        .lwip = {
            .init_fn = spi_rx_netstack_lwip_init_fn,
            .input_fn = spi_rx_netstack_lwip_input_fn
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
        .if_key = "spi_rx",
        .if_desc = "spi-rx if",
        .route_prio = 15,
        .bridge_info = NULL};

    esp_netif_config_t netif_config = {
        .base = &base_netif_config,
        .stack = &s_spi_netif_config,
        .driver = NULL};
    
    spi_netif_esp_netif_init(&spi_rx_netif, &netif_config, spi_rx_driver_post_attach);

    uint8_t mac[6] = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55};
    esp_netif_set_mac(spi_rx_netif, mac);

    
    ESP_ERROR_CHECK(esp_event_handler_instance_register(SPI_RX_EVENT, SPI_EVENT_START, spi_rx_default_action_sta_start, NULL, NULL));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, spi_rx_default_handler, NULL, NULL));
    printf("esp_event_handler_instance_register\n");
    ESP_ERROR_CHECK(esp_event_post(SPI_RX_EVENT, SPI_EVENT_START, NULL, 0, portMAX_DELAY));
    printf("esp_event_post\n");

    return ESP_OK;
}
///////////

err_t output_function(struct netif *netif, struct pbuf *p, const ip4_addr_t *ipaddr)
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

err_t linkoutput_function(struct netif *netif, struct pbuf *p)
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

esp_err_t esp_netif_spi_driver_transmit(void *h, void *buffer, size_t len)
{
    ESP_LOGI(TAG, "spi_netif_driver_transmit(void *h, void *buffer, size_t len) called");
    
    if (buffer==NULL) {
        ESP_LOGI(TAG, "buffer is null");
        return ESP_OK;
    }
    spi_payload_t p = {
        .id = 0,
        .ttl = SPI_PAYLOAD_TTL,
        .src_device_id = device_config_get_id(),
        .dst_device_id = DEVICE_ID_ANY,
        .buffer_type = SPI_PAYLOAD_TYPE_ESP_NETIF,
        .len = len,
    };
    memccpy(p.buffer, buffer, len, SPI_PAYLOAD_BUFFER_SIZE);
    return spi_payload_transmit(&p);
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
    ESP_LOGI(TAG, "Calling esp_netif_spi_post_attach_start(esp_netif_t * esp_netif, void *args)");
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
    printf("spi_tx_netif_init\n");
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
    
    spi_netif_esp_netif_init(&spi_tx_netif, &netif_config, spi_tx_driver_post_attach);

    ESP_ERROR_CHECK(esp_event_handler_instance_register(SPI_TX_EVENT, SPI_EVENT_START, spi_default_action_sta_start, NULL, NULL));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, spi_default_handler, NULL, NULL));
    ESP_ERROR_CHECK(esp_event_post(SPI_TX_EVENT, SPI_EVENT_START, NULL, 0, portMAX_DELAY));
    return ESP_OK;
}


esp_err_t spi_netif_init(void)
{
    ESP_ERROR_CHECK(spi_rx_netif_init());
    ESP_ERROR_CHECK(spi_tx_netif_init());
    return ESP_OK;
}
