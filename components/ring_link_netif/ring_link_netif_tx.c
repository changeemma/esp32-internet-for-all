#include "ring_link_netif_tx.h"

static const char* TAG = "==> ring_link_netif";

ESP_EVENT_DEFINE_BASE(RING_LINK_TX_EVENT);

static esp_netif_t *ring_link_tx_netif = NULL;

static const struct esp_netif_netstack_config netif_netstack_config = {
    .lwip = {
        .init_fn = ring_link_tx_netstack_lwip_init_fn,
        .input_fn = ring_link_tx_netstack_lwip_input_fn
    }
};

static const esp_netif_inherent_config_t netif_inherent_config = {
    .flags = ESP_NETIF_FLAG_AUTOUP,
    ESP_COMPILER_DESIGNATED_INIT_AGGREGATE_TYPE_EMPTY(mac)
    ESP_COMPILER_DESIGNATED_INIT_AGGREGATE_TYPE_EMPTY(ip_info)
    .get_ip_event = 0,
    .lost_ip_event = 0,
    .if_key = "ring_link_tx",
    .if_desc = "ring-link-tx if",
    .route_prio = 15,
    .bridge_info = NULL
};

static const esp_netif_config_t netif_config = {
    .base = &netif_inherent_config,
    .driver = NULL,
    .stack = &netif_netstack_config,
};

err_t ring_link_tx_netstack_lwip_init_fn(struct netif *netif)
{
    LWIP_ASSERT("netif != NULL", (netif != NULL));
    netif->name[0]= 's';
    netif->name[1] = 'p';
    netif->output = output_function;
    netif->linkoutput = linkoutput_function;
    netif->mtu = RING_LINK_NETIF_MTU;
    netif->hwaddr_len = ETH_HWADDR_LEN;
    netif->flags = NETIF_FLAG_BROADCAST | NETIF_FLAG_LINK_UP;
    return ERR_OK;
}

esp_netif_recv_ret_t ring_link_tx_netstack_lwip_input_fn(void *h, void *buffer, size_t len, void* l2_buff)
{
    ESP_LOGW(TAG, "ring_link_tx_netstack_lwip_input_fn() called, but this is a TX-only netif");
    // No packets should ever be delivered to the stack on this netif
    return ESP_NETIF_OPTIONAL_RETURN_CODE(ESP_OK);
}

static void ring_link_tx_default_action_start(void *arg, esp_event_base_t base, int32_t event_id, void *data)
{
    ESP_LOGI(TAG, "Calling ring_link_tx_default_action_start");
    const esp_netif_ip_info_t ip_info = config_get_tx_ip_info();

    ESP_ERROR_CHECK(esp_netif_set_ip_info(ring_link_tx_netif, &ip_info));
    esp_netif_action_start(ring_link_tx_netif, base, event_id, data);

    ESP_ERROR_CHECK(esp_netif_set_default_netif(ring_link_tx_netif));
}

esp_err_t ring_link_tx_netif_init(void)
{
    ESP_LOGI(TAG, "Calling ring_link_tx_netif_init");

    ring_link_tx_netif = ring_link_netif_new(&netif_config);
    
    ESP_ERROR_CHECK(ring_link_netif_set_mac(ring_link_tx_netif, RING_LINK_TX_NETIF_OFFSET));
    
    ESP_ERROR_CHECK(esp_event_handler_instance_register(RING_LINK_TX_EVENT, RING_LINK_EVENT_START, ring_link_tx_default_action_start, NULL, NULL));
    
    ESP_ERROR_CHECK(esp_event_post(RING_LINK_TX_EVENT, RING_LINK_EVENT_START, NULL, 0, portMAX_DELAY));
    return ESP_OK;
}
