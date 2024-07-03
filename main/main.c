#include "main.h"


void app_main(void)
{
    ESP_ERROR_CHECK(init_nvs());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    ESP_ERROR_CHECK(esp_netif_init());
    read_config();

    // Initializing spi/wifi drivers
    ESP_ERROR_CHECK(spi_init());
    ESP_ERROR_CHECK(test_spi(false));

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
