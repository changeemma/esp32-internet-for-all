// C libraries
#include <stdio.h>

// ESP32 libraries
#include "esp_event.h"
#include "esp_check.h"

// Private components
#include "config.h"
#include "spi.h"
#include "spi_netif.h"
#include "udp_spi.h"
#include "wifi.h"
#include "route.h"

#include "nvs.h"

// Test files
#include "test_board.h"
#include "test_integration.h"

#define TEST_ALL true

static const char *TAG = "==> main";


void app_main(void)
{
    ESP_ERROR_CHECK(init_nvs());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    ESP_ERROR_CHECK(esp_netif_init());
    device_config_setup();
    device_config_print();

    // Initializing spi/wifi drivers
    ESP_ERROR_CHECK(spi_init());
    //ESP_ERROR_CHECK(test_spi(false));

    ESP_ERROR_CHECK(wifi_init());

    // Initializing spi/wifi network interfaces
    ESP_ERROR_CHECK(spi_rx_netif_init());
    ESP_ERROR_CHECK(spi_tx_netif_init());
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
