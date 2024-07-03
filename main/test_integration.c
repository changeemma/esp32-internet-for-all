#include "test_integration.h"

static const char *TEST_TAG = "==> TEST";


void test_sending_upd_packet_fordwarding_to_the_spi_interface(){
    // Envia los paquetes directamente hacia la interfaz SPI.
    // Para eso se utiliza la funcion esp_netif_receive().
    // El paquete finalmente es enviado a la funcion output_function() del archivo /spi/network_spi.c
    
    u16_t port;
    esp_netif_t *spi_netif;

    spi_netif = get_spi_tx_netif();

    struct pbuf *p;
    port = get_active_spi_port();
    u8_t test_data[10] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    p = udp_create_test_packet_(10, port, port, ESP_IP4TOADDR(192, 168, 1,46), ESP_IP4TOADDR(192, 168, 1, 100), test_data);
    if (p==NULL){
        ESP_LOGI(TEST_TAG, "p is NULL\n");
        return;
    }
    ESP_LOGI(TEST_TAG, "sending packet IPv4 to SPI interface with esp_netif_receive()\n");
    esp_netif_receive(spi_netif, p, (size_t) 10, NULL);
}

void test_sending_upd_packet_fordwarding_to_the_wifi_interface(){
    // Envia los paquetes directamente hacia la capa de red de la interfaz Wi-Fi.
    // Por eso se utiliza la funcion ip4_input(). En caso que se quiera usar la 
    // funcion esp_netif_receive() se deben generar tramas ETHERNET.
    // El paquete finalmente es enviado a la funcion output_function() del archivo /spi/network_spi.c
    
    u16_t port;
    struct netif *esp_netif_imp;
    esp_netif_t *wifi_netif;

    wifi_netif = get_wifi_netif();
    esp_netif_imp = esp_netif_get_netif_impl(wifi_netif);

    struct pbuf *p;
    port = get_active_spi_port();
    u8_t test_data[10] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    p = udp_create_test_packet_(10, port, port, ESP_IP4TOADDR(192, 168, 60,10), ESP_IP4TOADDR(192, 168, 1, 100), test_data);
    ESP_LOGI(TEST_TAG, "sending packet IPv4 to Wi-Fi interface\n");
    ip4_input(p, esp_netif_imp);
}

void test_sending_upd_packet_ipv6_fordwarding_to_the_spi_interface(){
    // Envia los paquetes directamente hacia la capa de red de la interfaz SPI.
    // Para eso se utiliza la funcion esp_netif_receive().
    // El paquete finalmente es enviado a la funcion output_function() del archivo /spi/network_spi.c
    
    u16_t port;
    struct netif *esp_netif_imp;
    esp_netif_t *spi_netif;

    spi_netif = get_spi_tx_netif();
    esp_netif_imp = esp_netif_get_netif_impl(spi_netif);

    port = get_active_spi_port();
    ip6_addr_t src_addr, dest_addr;
    ip6addr_aton("2001:db8::1", &src_addr);
    ip6addr_aton("fe80::b2a1:a2ff:fea3:b5b6", &dest_addr);
    struct pbuf *udp_packet = udp_create_test_packet_ipv6(32, port, port, &dest_addr, &src_addr);

    ESP_LOGI(TEST_TAG, "sending packet IPv6 to SPI interface\n");
    esp_netif_receive(spi_netif, udp_packet, 10, NULL);
}

void test_send_upd_packet(){
    // Envia los paquetes directamente hacia la capa de red de la interfaz spi.
    // Se utiliza la funcion esp_netif_receive().
    // El paquete finalmente es enviado a la funcion recv_upd_data() que espera el UDP en el puerto asignado.
    
    u16_t port;
    struct netif *esp_netif_imp;
    esp_netif_t *spi_netif;

    spi_netif = get_spi_tx_netif();

    struct pbuf *p;
    port = get_active_spi_port();

    p = udp_create_test_packet(16, port, port, ESP_IP4TOADDR(127, 0, 0,2), ESP_IP4TOADDR(192, 168, 1, 100));
    
    ESP_LOGI(TEST_TAG, "sending packet IPv4 to SPI interface and receiving in recv_upd_data() IPv4\n");
    esp_netif_receive(spi_netif, p, (size_t) 10, NULL);
}

void test_send_upd_packet_ipv6(){
    // Envia los paquetes directamente hacia la capa de red de la interfaz spi.
    // Se utiliza la funcion esp_netif_imp->input().
    // El paquete finalmente es enviado a la funcion recv_upd_data() que espera el UDP en el puerto asignado.
 

    u16_t port;
    struct netif *esp_netif_imp;
    esp_netif_t *spi_netif;

    spi_netif = get_spi_tx_netif();
    esp_netif_imp = esp_netif_get_netif_impl(spi_netif);

    struct pbuf *p;
    port = get_active_spi_port_ipv6();

    ip6_addr_t src_addr, dest_addr;
    ip6addr_aton("2001:db8::1", &src_addr);
    ip6addr_aton("fe80::b2a1:a2ff:fea3:b5b6", &dest_addr);
    struct pbuf *udp_packet = udp_create_test_packet_ipv6(32, port, port, &dest_addr, &src_addr);
    ESP_LOGI(TEST_TAG, "sending packet IPv6 to SPI interface and receiving in recv_upd_data() IPv6\n");
    esp_netif_imp->input(udp_packet, esp_netif_imp);
}

void test_sending_upd_packet_fordwarding_to_the_spi_interface_with_input_ipv6(){
    // Envia los paquetes directamente hacia la capa de red de la interfaz SPI.
    // Para eso se utiliza la funcion esp_netif_imp->input().
    // El paquete finalmente es enviado a la funcion output_function() del archivo /spi/network_spi.c
    
    struct netif *esp_netif_imp;
    esp_netif_t *spi_netif;
    u16_t port = 555;
    ip6_addr_t src_addr, dest_addr;
    ip6addr_aton("2001:db8::1", &src_addr);
    ip6addr_aton("fe80::b2a1:ffff:fea3:a5b8", &dest_addr);
    
    struct pbuf *udp_packet = udp_create_test_packet_ipv6(32, port, port, &dest_addr, &src_addr);
    spi_netif = get_spi_tx_netif();
    esp_netif_imp = esp_netif_get_netif_impl(spi_netif);

    ESP_LOGI(TEST_TAG, "sending packet IPv6 to SPI interface with esp_netif_imp->input()\n");
    esp_netif_receive(spi_netif, udp_packet, (size_t) 10, NULL);

}


void test(int activate){
    
    if(activate==1){
        // printf("Begin Test 1 -------\n\n");
        // test_sending_upd_packet_fordwarding_to_the_spi_interface();
        // printf("End Test 1 -------\n\n");

        if (get_board_mode() == BOARD_MODE_ACCESS_POINT) {
            printf("Begin Test 2 -------\n\n");
            test_sending_upd_packet_fordwarding_to_the_wifi_interface();
            printf("End Test 2 -------\n\n");
        }
        
        // printf("Begin Test 3 -------\n\n");
        // test_send_upd_packet();
        // printf("End Test 3 -------\n\n");
        
        // printf("Begin Test 4 -------\n\n");
        // test_send_upd_packet_ipv6();
        // printf("End Test 4 -------\n\n");

        // printf("Begin Test 5 -------\n");
        // test_sending_upd_packet_ipv6_fordwarding_to_the_spi_interface();
        // printf("End Test 5 -------\n");

        // printf("Begin Test 6 -------\n\n");
        // test_sending_upd_packet_fordwarding_to_the_spi_interface_with_input_ipv6();
        // printf("End Test 6 -------\n\n");

    }
}