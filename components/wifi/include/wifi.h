#include <stdint.h>
#include <string.h>

#include "esp_event.h"
#include "esp_check.h"
#include "esp_log.h"
#include "esp_mac.h"
#include "esp_wifi.h"
#include "lwip/esp_netif_net_stack.h"
#include "esp_netif_net_stack.h"

#include "config.h"
#include "utils.h"


#define WIFI_AP_SSID CONFIG_ESP_WIFI_SSID
#define WIFI_AP_PASS CONFIG_ESP_WIFI_PASSWORD
#define WIFI_AP_CHANNEL CONFIG_ESP_WIFI_CHANNEL
#define WIFI_AP_MAX_STA_CONN CONFIG_ESP_MAX_STA_CONN


esp_err_t wifi_init(void);
void wifi_ap_netif_init(void);
void wifi_sta_netif_init(void);
esp_netif_t * get_wifi_netif(void);
esp_err_t wifi_netif_init(void);
