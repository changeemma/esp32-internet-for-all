#include "spi.h"
#include "esp_log.h"
#include "ring_link_payload.h"

#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"
#include "driver/spi_slave.h"
#include "esp_log.h"
#include "esp_timer.h"


static spi_device_handle_t s_spi_device_handle = {0};
static const char* TAG_SPI = "==> SPI";
static QueueHandle_t ring_link_queue = NULL;
static int64_t transaction_start_time = 0;

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

esp_err_t spi_transmit(void *p, size_t len) {
    
    ring_link_payload_t* payload = (ring_link_payload_t*)p;
    int64_t transaction_start_time_ = esp_timer_get_time();
    esp_err_t ret;
    ESP_LOGI(TAG_SPI, "Pre-transmit payload - Type: 0x%02x, ID: %d, TTL: %d", 
             payload->buffer_type, payload->id, payload->ttl);
             
    spi_transaction_t t = {
        .length = len * 8,
        .tx_buffer = p,
    };
    ret = spi_device_queue_trans(s_spi_device_handle, &t, portMAX_DELAY);
    int64_t end_time = esp_timer_get_time();
    int64_t duration = end_time - transaction_start_time_;
    ESP_LOGW(TAG_SPI, "Mensaje transmitido: en %lld μs", duration);

    return ret;
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
        ESP_LOGI(TAG_SPI, "SPI transaction complete - Received %d bits", t.trans_len);
        if (t.trans_len != len * 8) {
            ESP_LOGW(TAG_SPI, "Incomplete transaction! Expected %d bits", len * 8);
        }
    }
    return ret;
}


// Estructura de evento para comunicación interna
typedef struct {
    void *rx_data;
    size_t data_len;
} spi_rx_event_t;

// Colas
QueueHandle_t spi_internal_queue;   // Cola interna para comunicación entre ISR y tarea

// Callback cuando se inicia una transacción SPI
void spi_pre_transfer_callback(spi_slave_transaction_t *trans) {
    // No hacemos nada aquí, solo para detectar inicio de transacción
}

// Callback cuando finaliza una transacción SPI esclavo
void spi_post_transfer_callback(spi_slave_transaction_t *trans) {
    // Calcular longitud de datos recibidos
    transaction_start_time = esp_timer_get_time();
    size_t data_len = (trans->trans_len + 7) / 8;  // Convertir bits a bytes
    // Si se recibieron datos, notificar a la tarea de procesamiento
    if (data_len > 0) {
        spi_rx_event_t evt = {
            .rx_data = trans->rx_buffer,
            .data_len = data_len
        };
        
        // Enviar evento a la cola interna desde ISR
        BaseType_t xHigherPriorityTaskWoken = pdFALSE;
        xQueueSendFromISR(spi_internal_queue, &evt, &xHigherPriorityTaskWoken);
        
        if (xHigherPriorityTaskWoken) {
            portYIELD_FROM_ISR();
        }
    }
}

// Inicialización del SPI como receptor (esclavo)
esp_err_t spi_rx_init(void) {
    // Configuración del bus SPI
    spi_bus_config_t buscfg = {
        .mosi_io_num = SPI_RECEIVER_GPIO_MOSI,
        .miso_io_num = -1,
        .sclk_io_num = SPI_RECEIVER_GPIO_SCLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
    };
    
    // Configuración del esclavo SPI con callbacks
    spi_slave_interface_config_t slvcfg = {
        .mode = 0,
        .spics_io_num = SPI_RECEIVER_GPIO_CS,
        .queue_size = SPI_QUEUE_SIZE,
        .flags = 0,
        .post_setup_cb = spi_pre_transfer_callback,
        .post_trans_cb = spi_post_transfer_callback
    };
    
    // Inicializar el SPI esclavo
    ESP_ERROR_CHECK(spi_slave_initialize(SPI_RECEIVER_HOST, &buscfg, &slvcfg, SPI_DMA_CH_AUTO));
    ESP_LOGI(TAG_SPI, "SPI Receptor inicializado con interrupciones");
    
    return ESP_OK;
}

// Tarea para procesar eventos SPI recibidos
void spi_rx_process_task(void *pvParameters) {
    spi_rx_event_t evt;
    ring_link_payload_t *payload;
    
    ESP_LOGI(TAG_SPI, "Tarea de procesamiento SPI iniciada");
    
    // Encolar primera transacción para recepción
    payload = heap_caps_malloc(sizeof(ring_link_payload_t), MALLOC_CAP_DMA);
    if (payload != NULL) {
        memset(payload, 0, sizeof(ring_link_payload_t));
        
        spi_slave_transaction_t trans = {
            .rx_buffer = payload,
            .length = sizeof(ring_link_payload_t) * 8,
            .user = payload  // Guardar referencia al buffer
        };
        
        ESP_ERROR_CHECK(spi_slave_queue_trans(SPI_RECEIVER_HOST, &trans, portMAX_DELAY));
    }
    
    while (1) {
        // Esperar evento de recepción
        if (xQueueReceive(spi_internal_queue, &evt, portMAX_DELAY)) {
            // Convertir el buffer recibido a payload
            payload = (ring_link_payload_t *)evt.rx_data;
            
            if (payload != NULL) {
                // Imprimir información sobre el mensaje recibido
                ESP_LOGI(TAG_SPI, "Mensaje recibido - Tipo: 0x%02X, ID: %d, TTL: %d, Longitud: %d",
                        payload->buffer_type, payload->id, payload->ttl, payload->len);
                
                // Verificar si es un tipo de mensaje válido
                if (payload->buffer_type == 0x11 || payload->buffer_type == 0x80) {
                    // Crear una copia del payload para enviar a la cola
                    ring_link_payload_t *payload_copy = heap_caps_malloc(sizeof(ring_link_payload_t), MALLOC_CAP_DEFAULT);
                    if (payload_copy != NULL) {
                        memcpy(payload_copy, payload, sizeof(ring_link_payload_t));
                        int64_t end_time = esp_timer_get_time();
                        int64_t duration = end_time - transaction_start_time;
                        ESP_LOGW(TAG_SPI, "Mensaje recibido: en %lld μs", duration);
                        // Enviar a tu cola existente
                        if (xQueueSend(ring_link_queue, &payload_copy, pdMS_TO_TICKS(100)) != pdTRUE) {
                            ESP_LOGW(TAG_SPI, "Cola llena, payload descartado");
                            free(payload_copy);
                        }
                    }
                } else {
                    ESP_LOGW(TAG_SPI, "Tipo de mensaje no reconocido: 0x%02X", payload->buffer_type);
                }
            }
            
            // Encolar una nueva transacción
            ring_link_payload_t *new_payload = heap_caps_malloc(sizeof(ring_link_payload_t), MALLOC_CAP_DMA);
            if (new_payload != NULL) {
                memset(new_payload, 0, sizeof(ring_link_payload_t));
                
                spi_slave_transaction_t trans = {
                    .rx_buffer = new_payload,
                    .length = sizeof(ring_link_payload_t) * 8,
                    .user = new_payload  // Guardar referencia al buffer
                };
                
                esp_err_t ret = spi_slave_queue_trans(SPI_RECEIVER_HOST, &trans, portMAX_DELAY);
                if (ret != ESP_OK) {
                    ESP_LOGE(TAG_SPI, "Error al encolar nueva transacción: %d", ret);
                    free(new_payload);
                }
            }
            
            // Liberar el buffer anterior
            free(payload);
        }
    }
}

// Inicialización del sistema de recepción SPI
esp_err_t spi_receiver_init(QueueHandle_t **queue) {
    // Crear cola interna para comunicación entre ISR y tarea
    spi_internal_queue = xQueueCreate(40, sizeof(spi_rx_event_t));
    ring_link_queue = xQueueCreate(40, sizeof(ring_link_payload_t*));
    *queue = &ring_link_queue;
    if (ring_link_queue == NULL) {
        ESP_LOGE(TAG_SPI, "Failed to create queue");
        return ESP_FAIL;
    }
    // Inicializar hardware SPI para recepción
    ESP_ERROR_CHECK(spi_rx_init());
    ESP_ERROR_CHECK(spi_tx_init());
    
    // Crear tarea para procesar mensajes recibidos
    xTaskCreate(spi_rx_process_task, "spi_rx_process", 4096*4, NULL, (tskIDLE_PRIORITY + 10), NULL);
    
    ESP_LOGI(TAG_SPI, "Sistema de recepción SPI inicializado");
    return ESP_OK;
}