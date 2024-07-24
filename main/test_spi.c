#include "test_spi.h"


void test_spi_internal_message_any_sibling( void )
{
    spi_payload_t p = {
        .id = 1,
        .src_device_id = device_config_get_id(),
        .dst_device_id = DEVICE_ID_ANY,
        .buffer_type = SPI_PAYLOAD_TYPE_INTERNAL,
        .buffer = "Hi neighbor!",
        .ttl = SPI_PAYLOAD_TTL,
        .len = SPI_PAYLOAD_BUFFER_SIZE,
    };
    printf("\n\n---------------------------\n");
    printf("SPI_INTERNAL: Testing spi_transmit to any sibling(dst_device_id=%i, TTL=%i)\n", \
            p.dst_device_id, p.ttl);
    assert(spi_lowlevel_transmit_payload(&p) == ESP_OK);
}

void test_spi_internal_wrong_sibling( void )
{
    spi_payload_t p = {
        .id = 2,
        .src_device_id = device_config_get_id(),
        .dst_device_id = DEVICE_ID_NONE,
        .buffer_type = SPI_PAYLOAD_TYPE_INTERNAL,
        .buffer = "where is Waldo?",
        .ttl = SPI_PAYLOAD_TTL,
        .len = SPI_PAYLOAD_BUFFER_SIZE,
    };
    printf("\n\n---------------------------\n");
    printf("SPI_INTERNAL: Testing spi_transmit to wrong sibling(dst_device_id=%i, TTL=%i)\n", \
            p.dst_device_id, SPI_PAYLOAD_TTL);
    assert(spi_lowlevel_transmit_payload(&p) == ESP_OK);
}

void test_spi_internal_broadcast_to_siblings( void )
{
    const char msg[] = "hello world, this is a broadcast.";
    printf("\n\n---------------------------\n");
    printf("SPI_INTERNAL: Testing broadcast_to_siblings(\"%s\", %i)\n", msg, sizeof(msg));
    assert(broadcast_to_siblings(msg, sizeof(msg)) == true);
}

void test_spi_netif_message_any_sibling( void )
{
    spi_payload_t p = {
        .id = 3,
        .src_device_id = device_config_get_id(),
        .dst_device_id = DEVICE_ID_ANY,
        .buffer_type = SPI_PAYLOAD_TYPE_ESP_NETIF,
        .buffer = "this is a netif payload",
        .ttl = SPI_PAYLOAD_TTL,
        .len = SPI_PAYLOAD_BUFFER_SIZE,
    };
    printf("\n\n---------------------------\n");
    printf("SPI_NETIF: Testing spi_transmit to any sibling(dst_device_id=%i, TTL=%i)\n", \
            p.dst_device_id, SPI_PAYLOAD_TTL);
    assert(spi_lowlevel_transmit_payload(&p) == ESP_OK);
}

void test_spi_netif_bouncing_message( void )
{
    spi_payload_t p = {
        .id = 4,
        .src_device_id = device_config_get_id(),
        .dst_device_id = device_config_get_id(),
        .buffer_type = SPI_PAYLOAD_TYPE_ESP_NETIF,
        .buffer = "hi, i am back!",
        .ttl = SPI_PAYLOAD_TTL,
        .len = SPI_PAYLOAD_BUFFER_SIZE,
    };
    printf("\n\n---------------------------\n");
    printf("SPI_NETIF: Testing spi_transmit to myself (dst_device_id=%i, TTL=%i)\n", \
            p.dst_device_id, SPI_PAYLOAD_TTL);
    assert(spi_lowlevel_transmit_payload(&p) == ESP_OK);
}

void test_spi_run_all(void)
{
    vTaskDelay(3000 / portTICK_PERIOD_MS);
    test_spi_internal_message_any_sibling();
    vTaskDelay(3000 / portTICK_PERIOD_MS);
    test_spi_internal_wrong_sibling();
    vTaskDelay(3000 / portTICK_PERIOD_MS);
    test_spi_internal_broadcast_to_siblings();
    vTaskDelay(3000 / portTICK_PERIOD_MS);
    test_spi_netif_message_any_sibling();
    vTaskDelay(3000 / portTICK_PERIOD_MS);
    test_spi_netif_bouncing_message();
    vTaskDelay(3000 / portTICK_PERIOD_MS);
    printf("\n\n\n\n\n\n\n\n\n\n\n");
}