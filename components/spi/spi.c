#include "spi.h"
#include "esp_log.h"
#include "ring_link_payload.h"


static spi_device_handle_t s_spi_device_handle = {0};
static const char* TAG = "==> SPI";
static QueueHandle_t rx_queue = NULL;
static spi_slave_transaction_t transactions[SPI_QUEUE_SIZE];
static ring_link_payload_t *payload_buffers[SPI_QUEUE_SIZE];


static void spi_post_trans_cb(spi_slave_transaction_t *trans) {
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    payload_msg_t msg = {
        .payload = (ring_link_payload_t *)trans->rx_buffer,
        .trans = trans
    };
    xQueueSendFromISR(rx_queue, &msg, &xHigherPriorityTaskWoken);

    if (xHigherPriorityTaskWoken) portYIELD_FROM_ISR();
}

static esp_err_t spi_rx_init(QueueHandle_t *queue) {
    //Configuration for the SPI bus
    spi_bus_config_t buscfg={
        .mosi_io_num=SPI_RECEIVER_GPIO_MOSI,
        .miso_io_num=-1,
        .sclk_io_num=SPI_RECEIVER_GPIO_SCLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
    };

    //Configuration for the SPI slave interface
    spi_slave_interface_config_t slvcfg={
        .mode = 0,
        .spics_io_num = SPI_RECEIVER_GPIO_CS,
        .queue_size = SPI_QUEUE_SIZE,
        .post_trans_cb = spi_post_trans_cb,
    };
    rx_queue = *queue;

    //Initialize SPI slave interface
    ESP_ERROR_CHECK(spi_slave_initialize(SPI_RECEIVER_HOST, &buscfg, &slvcfg, SPI_DMA_CH_AUTO));
    for (int i = 0; i < SPI_QUEUE_SIZE; i++) {
        payload_buffers[i] = (ring_link_payload_t *)heap_caps_malloc(sizeof(ring_link_payload_t), MALLOC_CAP_DMA);
        assert(payload_buffers[i] != NULL);

        memset(&transactions[i], 0, sizeof(spi_slave_transaction_t));
        transactions[i].length = sizeof(ring_link_payload_t) * 8;
        transactions[i].rx_buffer = payload_buffers[i];

        esp_err_t ret = spi_slave_queue_trans(SPI_RECEIVER_HOST, &transactions[i], portMAX_DELAY);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Error en spi_slave_queue_trans #%d: %s", i, esp_err_to_name(ret));
            vTaskDelete(NULL);
        }
    }
    printf("INicializacion OK SPI");
    return ESP_OK;
}

static esp_err_t spi_tx_init() {
    //Configuration for the SPI bus
    spi_bus_config_t buscfg={
        .mosi_io_num=SPI_SENDER_GPIO_MOSI,
        .miso_io_num=-1,
        .sclk_io_num=SPI_SENDER_GPIO_SCLK,
        .quadwp_io_num=-1,
        .quadhd_io_num=-1
    };

    //Configuration for the SPI device on the other side of the bus
    spi_device_interface_config_t devcfg={
        .command_bits = 0,
        .address_bits = 0,
        .dummy_bits = 0,
        .clock_speed_hz = 8 * 1000 * 1000,  // 8 MHz
        .duty_cycle_pos = 128,             // 50% duty cycle
        .mode = 0,
        .spics_io_num = SPI_SENDER_GPIO_CS,
        .cs_ena_pretrans = 3,
        .cs_ena_posttrans = 10,
        .queue_size = SPI_QUEUE_SIZE
    };

    //Initialize the SPI bus and add the device we want to send stuff to.
    ESP_ERROR_CHECK(spi_bus_initialize(SPI_SENDER_HOST, &buscfg, SPI_DMA_CH_AUTO));
    ESP_ERROR_CHECK(spi_bus_add_device(SPI_SENDER_HOST, &devcfg, &s_spi_device_handle));
    return ESP_OK;
}

esp_err_t spi_init(QueueHandle_t *queue) {
    ESP_ERROR_CHECK(spi_rx_init(queue));
    ESP_ERROR_CHECK(spi_tx_init());
    return ESP_OK;
}

esp_err_t spi_transmit(void *p, size_t len) {
    ring_link_payload_t* payload = (ring_link_payload_t*)p;
    ESP_LOGI(TAG, "Pre-transmit payload - Type: 0x%02x, ID: %d, TTL: %d", 
             payload->buffer_type, payload->id, payload->ttl);
             
    spi_transaction_t t = {
        .length = len * 8,
        .tx_buffer = p,
    };
    return spi_device_transmit(s_spi_device_handle, &t);
}
