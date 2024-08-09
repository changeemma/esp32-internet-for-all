// C libraries
#include <stdio.h>

// ESP32 libraries
#include "esp_event.h"
#include "esp_check.h"

// Private components
#include "config.h"
#include "ring_link_payload.h"
#include "ring_link_lowlevel.h"
#include "ring_link_internal.h"
#include "ring_link_netif.h"
#include "udp_spi.h"
#include "wifi.h"
#include "route.h"

#include "nvs.h"

// Test files
#include "test_integration.h"
#include "test_spi.h"

#define TEST_ALL true

static const char *TAG = "==> main";


static void spi_receive_task( void *pvParameters )
{
    esp_err_t rc;
    ring_link_payload_t p;
    while (true) {
        memset(&p, 0, sizeof(ring_link_payload_t));
        rc = ring_link_lowlevel_receive_payload(&p);
        ESP_ERROR_CHECK_WITHOUT_ABORT(rc);
        if (rc != ESP_OK) continue;

        switch (p.buffer_type)
        {
        case RING_LINK_PAYLOAD_TYPE_INTERNAL:
            ESP_ERROR_CHECK_WITHOUT_ABORT(ring_link_handler(&p));
            break;
        case RING_LINK_PAYLOAD_TYPE_ESP_NETIF:
            // check if netif is up
            ESP_ERROR_CHECK_WITHOUT_ABORT(ring_link_netif_handler(&p));
            break;
        default:
            ESP_LOGE(TAG, "Unknown payload type: '%i'", p.buffer_type);
            break;
        }
    }
    vTaskDelete(NULL);
}

esp_err_t spi_init(void)
{
    ring_link_lowlevel_init();
    ring_link_init();

    BaseType_t ret = xTaskCreate(spi_receive_task, "spi_receive_task", 2048, NULL, (tskIDLE_PRIORITY + 2), NULL);

    if (ret != pdTRUE) {
        ESP_LOGE(TAG, "create flow control task failed");
        return ESP_FAIL;
    }
    
    return ESP_OK;
}

void app_main(void)
{
    ESP_ERROR_CHECK(init_nvs());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    ESP_ERROR_CHECK(esp_netif_init());
    device_config_setup();
    device_config_print();

    // Initializing spi/wifi drivers
    ESP_ERROR_CHECK(spi_init());
    test_spi_run_all();

    ESP_ERROR_CHECK(wifi_init());

    // Initializing spi/wifi network interfaces
    ESP_ERROR_CHECK(ring_link_netif_init());
    bind_udp_spi();
    ESP_ERROR_CHECK(wifi_netif_init());

    add_route("192.168.0.0", WIFI_GATEWAY, "255.255.0.0", "r_1");
    add_route("192.168.60.0", WIFI_GATEWAY, "255.255.255.128", "r_3");
    add_route("192.168.60.0", SPI_GATEWAY, "255.255.255.0", "r_2");
    print_route_table();

    test(TEST_ALL);

    rm_route("192.168.60.0", "255.255.254.0");
    print_route_table();
}
