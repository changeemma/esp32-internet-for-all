#include "spi.h"
#include "esp_log.h"
#include "ring_link_payload.h"


static spi_device_handle_t s_spi_device_handle = {0};

static const char* TAG = "==> SPI";
static QueueHandle_t s_rx_queue;
static TaskHandle_t xTaskToNotify = NULL;
int64_t rx_start_time;
int64_t rx_end_time;
int64_t rx_duration;

static void spi_queue_trans_task(void *arg) {

    while (true)
    {
        static uint8_t rx_buffer[400];  // Example size, adjust as needed

        spi_slave_transaction_t *trans = heap_caps_calloc(1, sizeof(spi_slave_transaction_t), MALLOC_CAP_INTERNAL);
        if (!trans) {
            ESP_LOGE(TAG, "Failed to allocate transaction");
            vTaskDelay(pdMS_TO_TICKS(100));
            continue;
        }

        trans->length = sizeof(rx_buffer) * 8;  // bits
        trans->rx_buffer = rx_buffer;
        

        xTaskToNotify = xTaskGetCurrentTaskHandle();
        spi_slave_queue_trans(SPI_RECEIVER_HOST, trans, portMAX_DELAY);

        // Wait until post_trans_cb notifies us
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        rx_duration = rx_end_time - rx_start_time;
        ESP_LOGI(TAG, "RECEPTION TIME: %lld μs", rx_duration);   
    }
}

static void spi_post_trans_cb(spi_slave_transaction_t *trans) {
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    rx_start_time = esp_timer_get_time();
    if (trans->length >= SPI_BUFFER_SIZE) {
        if (xQueueSendFromISR(s_rx_queue, &trans->rx_buffer, &xHigherPriorityTaskWoken) != pdTRUE) {
            ESP_LOGW(TAG, "Queue full, payload dropped");
        }
    }
    rx_end_time = esp_timer_get_time();
    vTaskNotifyGiveFromISR(xTaskToNotify, &xHigherPriorityTaskWoken);
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

static esp_err_t spi_rx_init() {
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
        .mode=0,
        .spics_io_num=SPI_RECEIVER_GPIO_CS,
        .queue_size=SPI_QUEUE_SIZE,
        .flags=0,
        //.post_setup_cb=spi_post_setup_cb,
        .post_trans_cb=spi_post_trans_cb
    };

    //Initialize SPI slave interface
    ESP_ERROR_CHECK(spi_slave_initialize(SPI_RECEIVER_HOST, &buscfg, &slvcfg, SPI_DMA_CH_AUTO));

    BaseType_t ret;
    
    ret = xTaskCreate(
        spi_queue_trans_task,
        "spi_queue_trans_task",
        16384 * 2,
        NULL,
        (tskIDLE_PRIORITY + 2),
        NULL
    );
    if (ret != pdTRUE) {
        ESP_LOGE(TAG, "Failed to create receive task");
        return ESP_FAIL;
    }
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
        .command_bits=0,
        .address_bits=0,
        .dummy_bits=0,
        .clock_speed_hz=SPI_FREQ,
        .duty_cycle_pos=128,        //50% duty cycle
        .mode=0,
        .spics_io_num=SPI_SENDER_GPIO_CS,
        .cs_ena_pretrans = 3,   
        .cs_ena_posttrans = 10,        //Keep the CS low 3 cycles after transaction, to stop slave from missing the last bit when CS has less propagation delay than CLK
        .queue_size=SPI_QUEUE_SIZE
    };

    //Initialize the SPI bus and add the device we want to send stuff to.
    ESP_ERROR_CHECK(spi_bus_initialize(SPI_SENDER_HOST, &buscfg, SPI_DMA_CH_AUTO));
    ESP_ERROR_CHECK(spi_bus_add_device(SPI_SENDER_HOST, &devcfg, &s_spi_device_handle));
    return ESP_OK;
}

esp_err_t spi_init(QueueHandle_t *rx_queue) {
    s_rx_queue = *rx_queue;
    ESP_ERROR_CHECK(spi_rx_init());
    ESP_ERROR_CHECK(spi_tx_init());
    return ESP_OK;
}

esp_err_t spi_transmit(void *p, size_t len) {
    // ring_link_payload_t* payload = (ring_link_payload_t*)p;
    // ESP_LOGI(TAG, "Pre-transmit payload - Type: 0x%02x, ID: %d, TTL: %d", 
    //          payload->buffer_type, payload->id, payload->ttl);
             
    spi_transaction_t t = {
        .length = len * 8,
        .tx_buffer = p,
    };
    return spi_device_transmit(s_spi_device_handle, &t);
}

esp_err_t spi_receive(void *p, size_t len)
{
    spi_slave_transaction_t t = {
        .rx_buffer = p,
        .length = len * 8,
    };
    esp_err_t ret = spi_slave_transmit(SPI_RECEIVER_HOST, &t, portMAX_DELAY);
    
    if (ret == ESP_OK) {
        // Verificar que realmente recibimos la cantidad correcta de datos
        ESP_LOGI(TAG, "SPI transaction complete - Received %d bits", t.trans_len);
        if (t.trans_len != len * 8) {
            ESP_LOGW(TAG, "Incomplete transaction! Expected %d bits", len * 8);
        }
    }
    return ret;
}