#include "spi_lowlevel.h"

static const char* TAG = "==> spi-lowlevel";

static SemaphoreHandle_t s_tx_semaphore_handle = NULL;
static spi_device_handle_t s_spi_device_handle = {0};


static esp_err_t spi_rx_init() {
    //Configuration for the SPI bus
    spi_bus_config_t buscfg={
        .mosi_io_num=RECEIVER_GPIO_MOSI,
        .miso_io_num=-1,
        .sclk_io_num=RECEIVER_GPIO_SCLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
    };

    //Configuration for the SPI slave interface
    spi_slave_interface_config_t slvcfg={
        .mode=0,
        .spics_io_num=RECEIVER_GPIO_CS,
        .queue_size=SPI_QUEUE_SIZE,
        .flags=0,
        //.post_setup_cb=spi_post_setup_cb,
        //.post_trans_cb=spi_post_trans_cb
    };

    //Initialize SPI slave interface
    ESP_ERROR_CHECK(spi_slave_initialize(SPI_RECEIVER_HOST, &buscfg, &slvcfg, SPI_DMA_CH_AUTO));
    return ESP_OK;
}

static esp_err_t spi_tx_init() {
    //Configuration for the SPI bus
    spi_bus_config_t buscfg={
        .mosi_io_num=SENDER_GPIO_MOSI,
        .miso_io_num=-1,
        .sclk_io_num=SENDER_GPIO_SCLK,
        .quadwp_io_num=-1,
        .quadhd_io_num=-1
    };

    //Configuration for the SPI device on the other side of the bus
    spi_device_interface_config_t devcfg={
        .command_bits=0,
        .address_bits=0,
        .dummy_bits=0,
        .clock_speed_hz=SPI_MASTER_FREQ_8M,
        .duty_cycle_pos=128,        //50% duty cycle
        .mode=0,
        .spics_io_num=SENDER_GPIO_CS,
        .cs_ena_posttrans=3,        //Keep the CS low 3 cycles after transaction, to stop slave from missing the last bit when CS has less propagation delay than CLK
        .queue_size=SPI_QUEUE_SIZE
    };

    //Initialize the SPI bus and add the device we want to send stuff to.
    ESP_ERROR_CHECK(spi_bus_initialize(SPI_SENDER_HOST, &buscfg, SPI_DMA_CH_AUTO));
    ESP_ERROR_CHECK(spi_bus_add_device(SPI_SENDER_HOST, &devcfg, &s_spi_device_handle));
    s_tx_semaphore_handle = xSemaphoreCreateMutex();
    return ESP_OK;
}

esp_err_t spi_lowlevel_init(void) {
    ESP_ERROR_CHECK(spi_rx_init());
    ESP_ERROR_CHECK(spi_tx_init());
    return ESP_OK;
}

esp_err_t spi_lowlevel_transmit(void *p, size_t len) {
    esp_err_t rc;
    spi_transaction_t t = {
        .length = len * 8,
        .tx_buffer = p,
    };
    if( xSemaphoreTake( s_tx_semaphore_handle, ( TickType_t ) 10 ) == pdTRUE )
    {
        rc = spi_device_transmit(s_spi_device_handle, &t);
        xSemaphoreGive( s_tx_semaphore_handle );
        return rc;
    }
    ESP_LOGE(TAG, "Could not adquire Mutex...");
    return ESP_FAIL;
}

esp_err_t spi_lowlevel_receive(void *p, size_t len)
{
    spi_slave_transaction_t t = {
        .rx_buffer = p,
        .length = len * 8,
    };
    return spi_slave_transmit(SPI_RECEIVER_HOST, &t, portMAX_DELAY);
}
