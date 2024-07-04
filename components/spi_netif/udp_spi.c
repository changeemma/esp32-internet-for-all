#include "udp_spi.h"

static const char *SPI_UDP_TAG = "==> spi-udp-if";
const u16_t port = 12345;
const u16_t port_ipv6 = 12346;

struct udp_rxdata
{
    u32_t rx_cnt;
    u32_t rx_bytes;
    struct udp_pcb *pcb;
};

void recv_upd_data(void *arg, struct udp_pcb *pcb, struct pbuf *p,
                      const ip_addr_t *addr, u16_t port)
{
    // struct udp_rxdata *ctr = (struct udp_rxdata *)arg;
    u8_t *payload_array = (u8_t *)(p->payload);
    printf("recv_upd_data: Printing payload\n");
    for (u16_t i = 0; i < ((u16_t) p->len); i++)
    {
        printf("%2x", payload_array[i]);
    }
    printf("\n");

    if (p != NULL)
    {
        pbuf_free(p);
    }
}

u16_t get_active_spi_port(void){
    return port;
}

u16_t get_active_spi_port_ipv6(void){
    return port_ipv6;
}

void bind_udp_to_netif(esp_netif_t *esp_netif, u16_t port)
{
    // Bind UPD
    struct udp_pcb *pcb;
    struct udp_rxdata ctr;
    struct netif *esp_netif_imp;
    ESP_LOGI(SPI_UDP_TAG, "bind udp");

    pcb = udp_new();
    esp_netif_imp = esp_netif_get_netif_impl(esp_netif);
    udp_bind(pcb, &esp_netif_imp->ip_addr, port);
    
    memset(&ctr, 0, sizeof(ctr));
    ctr.pcb = pcb;
    udp_recv(pcb, recv_upd_data, &ctr);
    ESP_LOGI(SPI_UDP_TAG, "bind udp finish");
}

void bind_udp_to_netif_ipv6(esp_netif_t *esp_netif, u16_t port)
{
    // Bind UPD
    struct udp_pcb *pcb;
    struct udp_rxdata ctr;
    struct netif *esp_netif_imp;
    ESP_LOGI(SPI_UDP_TAG, "bind udp ipv6");

    pcb = udp_new();
    esp_netif_imp = esp_netif_get_netif_impl(esp_netif);
    udp_bind(pcb, &esp_netif_imp->ip6_addr[0], port_ipv6);
    
    memset(&ctr, 0, sizeof(ctr));
    ctr.pcb = pcb;
    udp_recv(pcb, recv_upd_data, &ctr);
    ESP_LOGI(SPI_UDP_TAG, "bind udp finish ipv6");
}


void bind_udp_spi(void){
    ESP_LOGI(SPI_UDP_TAG, " binding udp\n");
    esp_netif_t *spi_netif;
    spi_netif = get_spi_tx_netif();
    bind_udp_to_netif(spi_netif, port);
    bind_udp_to_netif_ipv6(spi_netif, port_ipv6);
}
