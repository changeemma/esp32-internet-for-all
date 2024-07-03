#include "spi_rx_netif.h"

const char *SPI_NETIF_RX_TAG = "==> spi_rx_netif";

WORD_ALIGNED_ATTR char spi_rx_buffer[SPI_PAYLOAD_BUFFER_SIZE];
ESP_EVENT_DEFINE_BASE(SPI_RX_EVENT);

esp_netif_t *spi_rx_netif;

esp_netif_t * get_spi_rx_netif(void){
    return spi_rx_netif;
}

static void esp_netif_spi_receive_task(void *arg) {
    size_t len;
    esp_netif_t *netif = (esp_netif_t *) arg;
    struct pbuf *q;
    q = pbuf_alloc(PBUF_TRANSPORT, 500, PBUF_POOL);
    while (true) {
        ESP_LOGI(SPI_NETIF_RX_TAG, "esp_netif_spi_receive_task()...");
        len = spi_receive(spi_rx_buffer);
        if (len <= 0) {
            printf("an error ocurred receiving or empty data\n");
            continue;
        }
        printf("%d\n", len);
        ESP_LOGI(SPI_NETIF_RX_TAG, "Calling esp_netif_receive(netif, spi_rx_buffer, len, NULL)");
        q->payload = spi_rx_buffer;
        struct ip_hdr *iphdr = (struct ip_hdr *)q->payload;
        if((IPH_V(iphdr) == 4) || (IPH_V(iphdr) == 6)){
            q->next = NULL;
            q->len = lwip_ntohs(IPH_LEN(iphdr));
            q->tot_len = len/8;
            esp_netif_receive(spi_rx_netif, q, q->tot_len, NULL);
        }
    }
    vTaskDelete(NULL);
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
    ESP_LOGW(SPI_NETIF_RX_TAG, "spi_rx_driver_transmit(void *h, void *buffer, size_t len) called");
    
    if (buffer==NULL) {
        ESP_LOGW(SPI_NETIF_RX_TAG, "buffer is null");
        return ESP_OK;
    }
    return ESP_OK;
}

esp_err_t spi_rx_driver_post_attach(esp_netif_t * esp_netif, void * args)
{
    ESP_LOGI(SPI_NETIF_RX_TAG, "Calling esp_netif_spi_post_attach_start(esp_netif_t * esp_netif, void *args)");
    spi_netif_driver_t driver = (spi_netif_driver_t) args;
    driver->base.netif = esp_netif;
    esp_netif_driver_ifconfig_t driver_ifconfig = {
        .handle = driver,
        .transmit = spi_rx_driver_transmit,
        .driver_free_rx_buffer = NULL,
    };

    ESP_ERROR_CHECK(esp_netif_set_driver_config(esp_netif, &driver_ifconfig));

    BaseType_t ret = xTaskCreate(esp_netif_spi_receive_task, "receive_flow_ctl", 2048, esp_netif, (tskIDLE_PRIORITY + 2), NULL);

    if (ret != pdTRUE) {
        ESP_LOGE(SPI_NETIF_RX_TAG, "create flow control task failed");
        return ESP_FAIL;
    }
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
    
    spi_netif_init(&spi_rx_netif, &netif_config, spi_rx_driver_post_attach);

    uint8_t mac[6] = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55};
    esp_netif_set_mac(spi_rx_netif, mac);

    
    ESP_ERROR_CHECK(esp_event_handler_instance_register(SPI_RX_EVENT, SPI_EVENT_START, spi_rx_default_action_sta_start, NULL, NULL));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, spi_rx_default_handler, NULL, NULL));
    ESP_ERROR_CHECK(esp_event_post(SPI_RX_EVENT, SPI_EVENT_START, NULL, 0, portMAX_DELAY));
    return ESP_OK;
}