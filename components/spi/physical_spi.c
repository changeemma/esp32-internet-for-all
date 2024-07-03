#include "physical_spi.h"

const char *PHYSICAL_SPI_TAG = "==> spi-phy";

spi_device_handle_t spi_device_handle = {0};

esp_err_t spi_init() {
    ESP_ERROR_CHECK(spi_tx_init());
    ESP_ERROR_CHECK(spi_rx_init());
    return ESP_OK;
}

// SPI SENDER (master)
esp_err_t spi_tx_init() {
    ESP_LOGI(PHYSICAL_SPI_TAG, "Calling spi_tx_init()");

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
    ESP_ERROR_CHECK(spi_bus_add_device(SPI_SENDER_HOST, &devcfg, &spi_device_handle));

    ESP_LOGI(PHYSICAL_SPI_TAG, "Returning from init_spi_tx()");
    return ESP_OK;
}

void _spi_transmit(void * buffer, size_t len) {
    spi_transaction_t t = {0};
    t.length = len;
    t.tx_buffer = buffer;
    ESP_ERROR_CHECK(spi_device_transmit(spi_device_handle, &t));
}

void spi_transmit_new(void * buffer, size_t len) {
    ESP_LOGI(PHYSICAL_SPI_TAG, "Calling spi_transmit()");
    //ip4_debug_print((struct pbuf *)buffer);

    WORD_ALIGNED_ATTR char p[SPI_PAYLOAD_BUFFER_SIZE];
    size_t sent = 0;
    const size_t max_batch = sizeof(char) * SPI_PAYLOAD_BUFFER_SIZE;
    // if len>0?
    while(sent < len) {
        int batch = (len - sent) > max_batch ? max_batch : (len - sent);
        memcpy(p, buffer+sent, batch);
        //ip4_debug_print((struct pbuf *)p);
        _spi_transmit(p, batch);
        sent += batch;
    }
}

void spi_transmit(void * buffer, size_t len) {
    ESP_LOGI(PHYSICAL_SPI_TAG, "Calling spi_transmit()");
    // WORD_ALIGNED_ATTR char p[SPI_PAYLOAD_BUFFER_SIZE];
    spi_transaction_t t = {0};
    t.length = len * 8;
    printf("Bytes en formato hexadecimal:\n");
    t.tx_buffer = buffer;
    u8_t *payload_array = (u8_t *)(buffer);
    printf("Printing payload\n");
    for (u16_t i = 0; i < ((u16_t) len); i++)
    {
        printf("%2x;", payload_array[i]);
    }
    printf("\n");
    
    ESP_ERROR_CHECK_WITHOUT_ABORT(spi_device_transmit(spi_device_handle, &t));
}

// SPI RECEIVER (slave)
esp_err_t spi_rx_init() {
    ESP_LOGI(PHYSICAL_SPI_TAG, "Calling spi_rx_init()");

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
    ESP_LOGI(PHYSICAL_SPI_TAG, "Returning from init_spi_rx()");
    return ESP_OK;
}

size_t spi_receive(void *buffer) {
    ESP_LOGI(PHYSICAL_SPI_TAG, "Calling spi_receive()");
    esp_err_t err_rc;
    spi_slave_transaction_t t = {0};
    memset(buffer, 0, SPI_PAYLOAD_BUFFER_SIZE);
    t.rx_buffer = buffer;
    t.length = SPI_PAYLOAD_BUFFER_SIZE;
    err_rc = spi_slave_transmit(SPI_RECEIVER_HOST, &t, portMAX_DELAY);
    printf("Bytes en formato hexadecimal:\n");
    u8_t *payload_array = (u8_t *) buffer;
    printf("Printing payload\n");
    for (u16_t i = 0; i < t.trans_len/8; i++)
    {
        printf("%2x;", payload_array[i]);
    }
    printf("\n");
    
    ESP_ERROR_CHECK_WITHOUT_ABORT(err_rc);

    return err_rc == ESP_OK ? t.trans_len : -1;
}

