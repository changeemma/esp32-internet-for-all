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
    .flags = ESP_NETIF_FLAG_AUTOUP,
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
    struct ip_hdr *ip_header = (struct ip_hdr *)p->buffer;

    // Tamaño total del paquete IP
    u16_t ip_total_len = lwip_ntohs(IPH_LEN(ip_header));

    if (ip_total_len > p->len) {
        ESP_LOGW(TAG, "Discarding packet: IP total_len=%d > payload=%d", ip_total_len, p->len);
        return ESP_OK;
    }

    // Reservar pbuf adecuado
    struct pbuf *q = pbuf_alloc(PBUF_TRANSPORT, ip_total_len, PBUF_RAM);
    if (q == NULL) {
        ESP_LOGW(TAG, "Failed to allocate pbuf");
        return ESP_FAIL;
    }

    // Copiar datos en la cadena de pbufs
    if (pbuf_take(q, p->buffer, ip_total_len) != ERR_OK) {
        ESP_LOGE(TAG, "pbuf_take failed");
        pbuf_free(q);
        return ESP_FAIL;
    }

    // Pasar paquete a la netif
    esp_err_t error = esp_netif_receive(ring_link_rx_netif, q, q->tot_len, NULL);
    if (error != ESP_OK) {
        ESP_LOGW(TAG, "esp_netif_receive failed: %s", esp_err_to_name(error));
        pbuf_free(q);
    }

    return error;
}

err_t ring_link_rx_netstack_lwip_init_fn(struct netif *netif)
{
    LWIP_ASSERT("netif != NULL", (netif != NULL));
    netif->name[0]= 'r';
    netif->name[1] = 'x';
    netif->output = output_function;
    netif->linkoutput = linkoutput_function;
    netif->mtu = RING_LINK_NETIF_MTU;
    netif->hwaddr_len = ETH_HWADDR_LEN;
    netif->flags = NETIF_FLAG_BROADCAST | NETIF_FLAG_LINK_UP;
    return ERR_OK;
}

esp_netif_recv_ret_t ring_link_rx_netstack_lwip_input_fn(void *h, void *buffer, size_t len, void *l2_buff)
{
    struct netif *netif = h;
    err_t result;  // Variable para almacenar el código de retorno

    /* Verifica que el buffer y la interfaz sean válidos */
    if (unlikely(!buffer || !netif_is_up(netif))) {
        if (l2_buff) {
            esp_netif_free_rx_buffer(netif->state, l2_buff);
        }
        return ESP_NETIF_OPTIONAL_RETURN_CODE(ESP_FAIL);
    }

    /* Usa directamente el buffer recibido */
    struct pbuf *p = (struct pbuf *)buffer;

    /* Llama a la función de entrada y captura el resultado */
    result = netif->input(p, netif);

    /* Imprime el código de error si hay problemas */
    if (unlikely(result != ERR_OK)) {
        ESP_LOGE("ring_link", "netif->input error: %d", result);
        return ESP_NETIF_OPTIONAL_RETURN_CODE(ESP_FAIL);
    }

    /* Todo salió bien */
    return ESP_NETIF_OPTIONAL_RETURN_CODE(ESP_OK);
}

static void ring_link_rx_default_action_start(void *arg, esp_event_base_t base, int32_t event_id, void *data)
{
    ESP_LOGI(TAG, "Calling ring_link_rx_default_action_start");
    const esp_netif_ip_info_t ip_info = config_get_rx_ip_info();
    
    ESP_ERROR_CHECK(esp_netif_set_ip_info(ring_link_rx_netif, &ip_info));
    esp_netif_action_start(ring_link_rx_netif, base, event_id, data);
}


esp_err_t ring_link_rx_netif_init(void)
{
    ESP_LOGI(TAG, "Calling ring_link_rx_netif_init");
    ring_link_rx_netif = ring_link_netif_new(&netif_config);

    ESP_ERROR_CHECK(ring_link_netif_set_mac(ring_link_rx_netif, RING_LINK_RX_NETIF_OFFSET));
    
    ESP_ERROR_CHECK(esp_event_handler_instance_register(RING_LINK_RX_EVENT, RING_LINK_EVENT_START, ring_link_rx_default_action_start, NULL, NULL));
    
    ESP_ERROR_CHECK(esp_event_post(RING_LINK_RX_EVENT, RING_LINK_EVENT_START, NULL, 0, portMAX_DELAY));
    return ESP_OK;
}
