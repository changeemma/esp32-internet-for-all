// C libraries
#include <stdio.h>

// ESP32 libraries
#include "esp_event.h"
#include "esp_check.h"

// Private components
#include "config.h"
#include "ring_link.h"
#include "heartbeat.h"
#include "wifi.h"
#include "route.h"

#include "nvs.h"

// Test files
#include "test_spi.h"

#define TEST_ALL false

static const char *TAG = "==> main";


void app_main(void)
{
    ESP_ERROR_CHECK(init_nvs());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    ESP_ERROR_CHECK(esp_netif_init());
    device_config_setup();
    device_config_print();

    ESP_ERROR_CHECK(ring_link_init());
    test_spi_run_all();

    ESP_ERROR_CHECK(wifi_init());
    ESP_ERROR_CHECK(wifi_netif_init());

    // add_route("192.168.0.0", WIFI_GATEWAY, "255.255.0.0", "r_1");
    // add_route("192.168.60.0", WIFI_GATEWAY, "255.255.255.128", "r_3");
    // add_route("192.168.60.0", SPI_GATEWAY, "255.255.255.0", "r_2");
    print_route_table();


    // rm_route("192.168.60.0", "255.255.254.0");
    #ifdef CONFIG_RING_LINK_LOWLEVEL_IMPL_SPI
    printf("CONFIG_RING_LINK_LOWLEVEL_IMPL_SPI\n");
    #endif

    #ifdef CONFIG_RING_LINK_LOWLEVEL_IMPL_PHYSIM
    printf("CONFIG_RING_LINK_LOWLEVEL_IMPL_PHYSIM\n");
    #endif
    
    print_route_table();
    
    init_heartbeat();
}
