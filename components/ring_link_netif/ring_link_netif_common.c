#include "ring_link_netif_common.h"

static const char* TAG = "==> ring_link_netif_common";

static esp_err_t ring_link_driver_post_attach(esp_netif_t * esp_netif, void * args)
{
    ESP_LOGI(TAG, "Calling esp_netif_ring_link_post_attach_start(esp_netif_t * esp_netif, void *args)");
    ring_link_netif_driver_t driver = (ring_link_netif_driver_t) args;
    driver->base.netif = esp_netif;
    esp_netif_driver_ifconfig_t driver_ifconfig = {
        .handle = driver,
        .transmit = ring_link_driver_transmit,
        .transmit_wrap = ring_link_driver_transmit_wrap,
        .driver_free_rx_buffer = NULL,
    };

    ESP_ERROR_CHECK(esp_netif_set_driver_config(esp_netif, &driver_ifconfig));

    return ESP_OK;
}

esp_netif_t* ring_link_netif_new(const esp_netif_config_t* config)
{
    ESP_LOGD(TAG, "allocating new esp_netif");
    esp_netif_t *netif = esp_netif_new(config);
    assert(netif);

    ESP_LOGD(TAG, "allocating new netif_driver");
    ring_link_netif_driver_t driver = calloc(1, sizeof(struct ring_link_netif_driver));
    assert(driver);

    driver->base.post_attach = ring_link_driver_post_attach;

    ESP_ERROR_CHECK(esp_netif_attach(netif, driver));
    return netif;
}

esp_err_t ring_link_netif_set_mac(esp_netif_t *netif, int offset)
{
    uint8_t mac[6];
    esp_err_t ret = esp_read_mac(mac, ESP_MAC_WIFI_SOFTAP);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Error getting MAC address");
        return ret;
    }

    mac[0] |= 0x02;  // locally administered (bit 1 of first byte = 1)
    mac[5] += offset;  // offset 
    
    return esp_netif_set_mac(netif, mac);
}

err_t linkoutput_function(struct netif *netif, struct pbuf *p)
{
    esp_netif_t *esp_netif = esp_netif_get_handle_from_netif_impl(netif);
    if (!esp_netif) {
        LWIP_DEBUGF(NETIF_DEBUG, ("linkoutput: esp-netif is NULL (netif=%p pbuf=%p len=%d)\n",
                                  netif, p, p->len));
        return ERR_IF;
    }

    esp_err_t ret;
    if (p->next == NULL) {
        // Fast path: single contiguous pbuf
        // if (p->len > RING_LINK_PAYLOAD_BUFFER_SIZE) {
        //     ESP_LOGE(TAG, "buffer length %zu exceeds max %u", p->len, RING_LINK_PAYLOAD_BUFFER_SIZE);
        //     return ERR_MEM;
        // }
        ret = esp_netif_transmit(esp_netif, p->payload, p->len);
    } else {
        // if (p->tot_len > RING_LINK_PAYLOAD_BUFFER_SIZE) {
        //     ESP_LOGE(TAG, "buffer length %zu exceeds max %u", p->tot_len, RING_LINK_PAYLOAD_BUFFER_SIZE);
        //     return ERR_MEM;
        // }
        // Slow path: chained pbuf → copy into contiguous buffer
        LWIP_DEBUGF(PBUF_DEBUG, ("linkoutput: pbuf chain detected, copying into RAM buffer\n"));
        struct pbuf *q = pbuf_alloc(PBUF_TRANSPORT, p->tot_len, PBUF_RAM);
        if (!q) {
            return ERR_MEM;
        }
        pbuf_copy(q, p);
        ret = esp_netif_transmit(esp_netif, q->payload, q->len);
        pbuf_free(q);
    }

    switch (ret) {
        case ESP_OK:          return ERR_OK;
        case ESP_ERR_NO_MEM:  return ERR_MEM;
        default:              return ERR_IF;
    }
}

err_t output_function(struct netif *netif, struct pbuf *p, const ip4_addr_t *ipaddr)
{
    ESP_LOGD(TAG, "Output function - len: %u, tot_len: %u", p->len, p->tot_len);

    if (p->len >= sizeof(struct ip_hdr)) {
        struct ip_hdr *iphdr = (struct ip_hdr *)p->payload;
        if (IPH_V(iphdr) == 4) {
            return netif->linkoutput(netif, p);
        } else {
            ESP_LOGW(TAG, "Unsupported packet version (v=%d)", IPH_V(iphdr));
            return ERR_VAL;
        }
    } else {
        ESP_LOGW(TAG, "Truncated packet in output_function (len=%u)", p->len);
        return ERR_BUF;
    }
}

esp_err_t ring_link_driver_transmit(void *h, void *buffer, size_t len)
{
    static int id_counter = 0;
    if (buffer == NULL || len == 0) {
        ESP_LOGE(TAG, "buffer is NULL");
        return ESP_ERR_INVALID_ARG;
    }

    if (len > RING_LINK_PAYLOAD_BUFFER_SIZE) {
        ESP_LOGE(TAG, "buffer length %zu exceeds max %u", len, RING_LINK_PAYLOAD_BUFFER_SIZE);
        return ESP_ERR_INVALID_SIZE;
    }

    ring_link_payload_t p = {
        .id          = id_counter++,
        .ttl         = RING_LINK_PAYLOAD_TTL,
        .src_id      = config_get_id(),
        .dst_id      = CONFIG_ID_ANY,
        .buffer_type = RING_LINK_PAYLOAD_TYPE_ESP_NETIF,
        .len         = len,
    };

    memcpy(p.buffer, buffer, len);
    return ring_link_lowlevel_transmit_payload(&p);
}

esp_err_t ring_link_driver_transmit_wrap(void *h, void *buffer, size_t len, void *netstack_buffer)
{
    if (len == 0) {
        ESP_LOGW(TAG, "Ignoring zero-length TX packet");
        return ESP_OK;
    }
    return ring_link_driver_transmit(h, buffer, len);
}