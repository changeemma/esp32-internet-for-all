#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/spi_slave.h"
#include "driver/spi_master.h"
#include "esp_log.h"
#include "sdkconfig.h"

#define TAG "SPI_RING_PAYLOAD"

// Pines
#define SPI_SENDER_GPIO_MOSI 23
#define SPI_SENDER_GPIO_SCLK 18
#define SPI_SENDER_GPIO_CS   5

#define SPI_RECEIVER_GPIO_MOSI 13
#define SPI_RECEIVER_GPIO_SCLK 14
#define SPI_RECEIVER_GPIO_CS   15

#define SPI_SENDER_HOST    VSPI_HOST
#define SPI_RECEIVER_HOST  HSPI_HOST

#define RING_LINK_LOWLEVEL_BUFFER_SIZE 240
#define PADDING_SIZE(x) (4 - ((x) % 4))
#define RING_LINK_PAYLOAD_BUFFER_SIZE (RING_LINK_LOWLEVEL_BUFFER_SIZE + PADDING_SIZE(RING_LINK_LOWLEVEL_BUFFER_SIZE))
#define RING_LINK_PAYLOAD_TTL 4
#define SPI_QUEUE_SIZE 5

typedef uint8_t config_id_t;
typedef uint8_t ring_link_payload_id_t;

typedef enum __attribute__((__packed__)) {
    RING_LINK_PAYLOAD_TYPE_INTERNAL = 0x11,
    RING_LINK_PAYLOAD_TYPE_ESP_NETIF = 0x80,
} ring_link_payload_buffer_type_t;

typedef struct __attribute__((__packed__)) {
    ring_link_payload_id_t id;
    ring_link_payload_buffer_type_t buffer_type;
    uint8_t len;
    uint8_t ttl;
    config_id_t src_id;
    config_id_t dst_id;
    char buffer[RING_LINK_PAYLOAD_BUFFER_SIZE];
} ring_link_payload_t;

#define MAX_MSG_SIZE sizeof(ring_link_payload_t)

// === Cola ===
typedef struct {
    ring_link_payload_t *payload;
    spi_slave_transaction_t *trans;
} spi_payload_msg_t;

static QueueHandle_t rx_queue = NULL;
static spi_slave_transaction_t transactions[SPI_QUEUE_SIZE];
static ring_link_payload_t *payload_buffers[SPI_QUEUE_SIZE];


// === Callback desde ISR ===
static void spi_post_trans_cb(spi_slave_transaction_t *trans) {
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    spi_payload_msg_t msg = {
        .payload = (ring_link_payload_t *)trans->rx_buffer,
        .trans = trans
    };

    xQueueSendFromISR(rx_queue, &msg, &xHigherPriorityTaskWoken);

    if (xHigherPriorityTaskWoken) portYIELD_FROM_ISR();
}

// === Inicializar receptor SPI ===
// Buffers DMA
static ring_link_payload_t *payload_buffers[SPI_QUEUE_SIZE];

void dump_payload(const ring_link_payload_t *p) {
    ESP_LOGI(TAG, "Payload recibido:");
    ESP_LOGI(TAG, "  ID: %d | Tipo: 0x%02X | TTL: %d | len: %d", p->id, p->buffer_type, p->ttl, p->len);
    ESP_LOGI(TAG, "  src: %d → dst: %d", p->src_id, p->dst_id);
    ESP_LOG_BUFFER_HEX("  buffer", p->buffer, p->len);
}

void spi_slave_task(void *arg) {
    spi_bus_config_t buscfg = {
        .mosi_io_num = SPI_RECEIVER_GPIO_MOSI,
        .miso_io_num = -1,
        .sclk_io_num = SPI_RECEIVER_GPIO_SCLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
    };

    spi_slave_interface_config_t slvcfg = {
        .mode = 0,
        .spics_io_num = SPI_RECEIVER_GPIO_CS,
        .queue_size = SPI_QUEUE_SIZE,
        .post_trans_cb = spi_post_trans_cb,
    };

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

    vTaskDelete(NULL);
}


// === Procesar el payload ===
void process_task(void *arg) {
    spi_payload_msg_t msg;
    while (1) {
        if (xQueueReceive(rx_queue, &msg, portMAX_DELAY)) {
            dump_payload(msg.payload);

            // Reencolar transacción para recibir el próximo
            ESP_ERROR_CHECK(spi_slave_queue_trans(SPI_RECEIVER_HOST, msg.trans, portMAX_DELAY));
        }
    }
}

// === Master ===
static spi_device_handle_t spi_handle = NULL;

static void spi_master_post_cb(spi_transaction_t *trans) {
    // ESP_LOGI(TAG, "Transacción SPI master completada (len = %d bytes)", trans->length / 8);
}


void spi_master_init() {
    spi_bus_config_t buscfg = {
        .mosi_io_num = SPI_SENDER_GPIO_MOSI,
        .miso_io_num = -1,
        .sclk_io_num = SPI_SENDER_GPIO_SCLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
    };

    spi_device_interface_config_t devcfg = {
        .command_bits = 0,
        .address_bits = 0,
        .dummy_bits = 0,
        .clock_speed_hz = 8 * 1000 * 1000,  // 8 MHz
        .duty_cycle_pos = 128,             // 50% duty cycle
        .mode = 0,
        .spics_io_num = SPI_SENDER_GPIO_CS,
        .cs_ena_pretrans = 3,
        .cs_ena_posttrans = 10,
        .queue_size = 1,
        .post_cb = spi_master_post_cb,     // <-- acá va el callback
    };


    ESP_ERROR_CHECK(spi_bus_initialize(SPI_SENDER_HOST, &buscfg, SPI_DMA_CH_AUTO));
    ESP_ERROR_CHECK(spi_bus_add_device(SPI_SENDER_HOST, &devcfg, &spi_handle));
}

// === Enviar un payload personalizado desde el master ===
esp_err_t send_payload(ring_link_payload_t *p) {
    spi_transaction_t t = {
        .length = sizeof(ring_link_payload_t) * 8,
        .tx_buffer = p
    };
    return spi_device_transmit(spi_handle, &t);
}

// === Ejemplo de envío ===
void example_sender_task(void *arg) {
    char * mensaje = "Hola ring payload";
    ring_link_payload_t payload = {
        .id = 1,
        .buffer_type = RING_LINK_PAYLOAD_TYPE_ESP_NETIF,
        .len = strlen(mensaje) +1,
        .ttl = 4,
        .src_id = 2,
        .dst_id = 3
    };
    memcpy(payload.buffer, mensaje, payload.len);

    while (1) {
        send_payload(&payload);
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}

// === MAIN ===
void app_main(void) {
    rx_queue = xQueueCreate(SPI_QUEUE_SIZE, sizeof(spi_payload_msg_t));
    assert(rx_queue != NULL);  // ← esto es crítico para evitar uso de puntero NULL

    spi_master_init();

    xTaskCreate(spi_slave_task, "spi_slave_task", 4096, NULL, 10, NULL);
    xTaskCreate(process_task, "process_task", 4096, NULL, 9, NULL);
    xTaskCreate(example_sender_task, "sender_task", 2048, NULL, 8, NULL);
}
