#include "wifi.h"

static const char *WIFI_TAG = "==> wifi-if";
esp_netif_t *wifi_netif;

esp_netif_t * get_wifi_netif(void){
    return wifi_netif;
}

static void wifi_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    if (event_base != WIFI_EVENT) {
        return;
    }
    switch (event_id)
    {
    case WIFI_EVENT_STA_START:
        esp_wifi_connect();
        break;
    case WIFI_EVENT_AP_STACONNECTED:
        wifi_event_ap_staconnected_t *event_c = (wifi_event_ap_staconnected_t *) event_data;
        ESP_LOGI(WIFI_TAG, "station " MACSTR " join, AID=%d", MAC2STR(event_c->mac), event_c->aid);
        break;
    case WIFI_EVENT_AP_STADISCONNECTED:
        wifi_event_ap_stadisconnected_t *event_d = (wifi_event_ap_stadisconnected_t *) event_data;
        ESP_LOGI(WIFI_TAG, "station " MACSTR " leave, AID=%d", MAC2STR(event_d->mac), event_d->aid);
        break;
    case WIFI_EVENT_AP_START:
        const esp_ip_addr_t wifi_ip6_addr = ESP_IP6ADDR_INIT(0xfe800000, 0x00000000, 0xb2a1a2ff, 0xfea3a4a5);
        esp_netif_set_ip6_linklocal(wifi_netif, wifi_ip6_addr);
        break;
    default:
        break;
    }
}

esp_err_t wifi_init(void) {
    /* Initialize WiFi Allocate resource for WiFi driver, such as WiFi control structure, 
    RX/TX buffer, WiFi NVS structure etc. This WiFi also starts WiFi task. */
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    
    return ESP_OK;
}

esp_err_t wifi_netif_init(void)
{
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &wifi_event_handler,
                                                        NULL,
                                                        NULL));
    if (get_board_mode() == BOARD_MODE_ACCESS_POINT) {
        ESP_LOGI(WIFI_TAG, "wifi_netif_init: I'm AP! Configuring as AP.");
        wifi_ap_netif_init();
    } else {
        ESP_LOGI(WIFI_TAG, "wifi_netif_init: I'm not root! I'm gonna be a STAr!");
        wifi_sta_netif_init();
    }
    return esp_wifi_start();
}

void wifi_sta_netif_init(void) {
    esp_netif_config_t cfg = ESP_NETIF_DEFAULT_WIFI_STA();
    wifi_netif = esp_netif_new(&cfg);
    assert(wifi_netif);
    
    ESP_ERROR_CHECK(esp_netif_attach_wifi_station(wifi_netif));
    ESP_ERROR_CHECK(esp_wifi_set_default_wifi_sta_handlers());

    wifi_config_t wifi_sta_config = {
        .sta = {
            .ssid = WIFI_AP_SSID,
            .password = WIFI_AP_PASS,
            .scan_method = WIFI_ALL_CHANNEL_SCAN,
            .failure_retry_cnt = 3,
            .sae_pwe_h2e = WPA3_SAE_PWE_BOTH,
        },
    };
    wifi_sta_config.sta.threshold.authmode = strlen(WIFI_AP_PASS) == 0? WIFI_AUTH_OPEN : WIFI_AUTH_WPA_WPA2_PSK;

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_sta_config) );

    ESP_LOGI(WIFI_TAG, "wifi_sta_netif_init finished.");
}

void wifi_ap_netif_init(void) {

    esp_netif_ip_info_t _g_esp_netif_soft_ap_ip = {
        .ip = {.addr = ESP_IP4TOADDR(192, 168, 4, 1)},
        .gw = {.addr = ESP_IP4TOADDR(192, 168, 4, 1)},
        .netmask = {.addr = ESP_IP4TOADDR(255, 255, 255, 0)},
    };

    esp_netif_config_t cfg = ESP_NETIF_DEFAULT_WIFI_AP();
    wifi_netif = esp_netif_new(&cfg);
    assert(wifi_netif);

    ESP_ERROR_CHECK(esp_netif_attach_wifi_ap(wifi_netif));
    ESP_ERROR_CHECK(esp_wifi_set_default_wifi_ap_handlers());

    wifi_config_t wifi_ap_config = {
        .ap = {
            .ssid = WIFI_AP_SSID,
            .ssid_len = strlen(WIFI_AP_SSID),
            .channel = WIFI_AP_CHANNEL,
            .password = WIFI_AP_PASS,
            .max_connection = WIFI_AP_MAX_STA_CONN,
            .pmf_cfg = {  .required = false, },
        },
    };
    wifi_ap_config.ap.authmode = strlen(WIFI_AP_PASS) == 0? WIFI_AUTH_OPEN : WIFI_AUTH_WPA_WPA2_PSK;

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_ap_config));

    ESP_LOGI(WIFI_TAG, "Wi-Fi AP SSID:%s password:%s channel:%d", WIFI_AP_SSID, WIFI_AP_PASS, WIFI_AP_CHANNEL);
    ESP_LOGI(WIFI_TAG, "wifi_ap_netif_init finished.");
}

