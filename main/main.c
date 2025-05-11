// Main original
// C libraries
#include <stdio.h>

// ESP32 libraries
#include "esp_event.h"
#include "esp_check.h"

// Private components
#include "config.h"
#include "ring_link.h"
#include "wifi.h"
#include "route.h"
#include "heartbeat.h"

#include "nvs.h"

// Test files
#include "test_spi.h"

#define TEST_ALL false

static const char *TAG = "==> main";


// ----Main original

#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/spi_slave.h"
#include "driver/spi_master.h"
#include "esp_log.h"
#include "sdkconfig.h"

// #define TAG "SPI_RING_PAYLOAD"

// calculo de tiempos
static int64_t spi_to_spi_start_time = 0;
static int64_t spi_to_spi_end_time = 0;
static ring_link_payload_id_t spi_to_spi_id = 0;

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
#define SPI_QUEUE_SIZE 80

#define MAX_MSG_SIZE sizeof(ring_link_payload_t)
#define MAX_BROADCASTS_IN_FLIGHT 8

static SemaphoreHandle_t spi_mutex = NULL;


typedef struct {
    ring_link_payload_id_t id;
    TaskHandle_t task;
    bool active;
} broadcast_entry_t;

static broadcast_entry_t broadcast_registry[MAX_BROADCASTS_IN_FLIGHT];


esp_err_t register_broadcast(ring_link_payload_id_t id, TaskHandle_t task) {
    for (int i = 0; i < MAX_BROADCASTS_IN_FLIGHT; i++) {
        if (!(broadcast_registry[i].active)) {
            broadcast_registry[i].id = id;
            broadcast_registry[i].task = task;
            broadcast_registry[i].active = true;
            return ESP_OK;
        }
    }
    return ESP_OK;

    ESP_LOGW(TAG, "No hay espacio para registrar nuevo broadcast en vuelo");
    return ESP_ERR_NO_MEM;
}
void init_broadcast_registry() {
    for (int i = 0; i < MAX_BROADCASTS_IN_FLIGHT; i++) {
        broadcast_registry[i].active = false;
        broadcast_registry[i].id = 0;
        broadcast_registry[i].task = NULL;
    }
}


// === Cola ===
typedef struct {
    ring_link_payload_t *payload;
    spi_slave_transaction_t *trans;
} spi_payload_msg_t;

static QueueHandle_t rx_queue = NULL;
static spi_slave_transaction_t transactions[SPI_QUEUE_SIZE];
static ring_link_payload_t *payload_buffers[SPI_QUEUE_SIZE];



QueueHandle_t internal_queue;
QueueHandle_t esp_netif_queue;
static spi_device_handle_t spi_handle = NULL;

esp_err_t send_payload(ring_link_payload_t *p) {
    // int64_t send_payload_time_init = esp_timer_get_time();
    
    if (xSemaphoreTake(spi_mutex, pdMS_TO_TICKS(1000)) != pdTRUE) {
        ESP_LOGE(TAG, "No se pudo tomar el mutex del SPI");
        return ESP_ERR_TIMEOUT;
    }


    spi_transaction_t t = {
        .length = sizeof(ring_link_payload_t) * 8,
        .tx_buffer = p
    };
    esp_err_t ret = spi_device_transmit(spi_handle, &t);
    xSemaphoreGive(spi_mutex);
    // int64_t send_payload_time_fin = esp_timer_get_time();

    // int64_t total_cycle_us = send_payload_time_fin-send_payload_time_init;
    // printf("[send_payload] payload_id=%d tomó %lld us (%.2f ms)\n", p->id, total_cycle_us, total_cycle_us / 1000.0);


    // if (spi_to_spi_start_time > 0 && p->id == spi_to_spi_id) {  // ✅ Confirmar mismo ID
        // int64_t spi_to_spi_end_time = esp_timer_get_time();
        // int64_t total_cycle_us = spi_to_spi_end_time - spi_to_spi_start_time;
        // ESP_LOGI(TAG, "[CICLO COMPLETO] payload_id=%d SPI→TCP/IP→SPI tomó %lld us (%.2f ms)", 
                //  spi_to_spi_id, total_cycle_us, total_cycle_us / 1000.0);
        // printf("[CICLO COMPLETO] payload_id=%d SPI→TCP/IP→SPI tomó %lld us (%.2f ms)\n", spi_to_spi_id, total_cycle_us, total_cycle_us / 1000.0);


        // Reset
    //     spi_to_spi_start_time = 0;
    //     spi_to_spi_id = 0;
    // }
    return ret;
}

// === netif TX ===
#include "ring_link_netif_tx.h"

// static const char* TAG = "==> ring_link_netif";

ESP_EVENT_DEFINE_BASE(RING_LINK_TX_EVENT);

static esp_netif_t *ring_link_tx_netif = NULL;
static ring_link_payload_id_t s_id_counter_tx = 0;


static err_t output_function(struct netif *netif, struct pbuf *p, const ip4_addr_t *ipaddr)
{
    // ESP_LOGI(TAG, "Calling output_function(struct netif *netif, struct pbuf *p, const ip4_addr_t *ipaddr)");
    // printf("len %u\n", p->len);
    // printf("total len %u\n", p->tot_len);
    ESP_LOGI(TAG, "Output function - len: %zu, tot_len: %zu", p->len, p->tot_len);
    struct ip_hdr *iphdr = (struct ip_hdr *)p->payload;
    if((IPH_V(iphdr) == 4) || (IPH_V(iphdr) == 6)){
        netif->linkoutput(netif, p);
    }else {
        ESP_LOGW(TAG, "Non-IP packet received!");
    }
    return ESP_OK;
}


static err_t linkoutput_function(struct netif *netif, struct pbuf *p)
{
    // ESP_LOGI(TAG, "Calling linkoutput_function(struct netif *netif, struct pbuf *p)");
    ring_link_payload_t p_out = {
        .id = s_id_counter_tx++,
        .ttl = RING_LINK_PAYLOAD_TTL,
        .src_id = config_get_id(),
        .dst_id = CONFIG_ID_ANY,
        .buffer_type = RING_LINK_PAYLOAD_TYPE_ESP_NETIF,
        .len = p->tot_len,
    };
    pbuf_copy_partial(p, p_out.buffer, p->tot_len, 0);
    send_payload(&p_out);
    return ERR_OK;
}

err_t ring_link_tx_netstack_init_fn(struct netif *netif)
{
    LWIP_ASSERT("netif != NULL", (netif != NULL));
    netif->name[0]= 's';
    netif->name[1] = 'p';
    netif->output = output_function;
    netif->linkoutput = linkoutput_function;
    netif->mtu = RING_LINK_PAYLOAD_BUFFER_SIZE;
    netif->hwaddr_len = ETH_HWADDR_LEN;
    netif->flags = NETIF_FLAG_BROADCAST | NETIF_FLAG_LINK_UP;
    return ERR_OK;
}


esp_netif_recv_ret_t ring_link_tx_netstack_input_fn(void *h, void *buffer, size_t len, void* l2_buff)
{
    return ESP_NETIF_OPTIONAL_RETURN_CODE(ESP_OK);
}

static const struct esp_netif_netstack_config netif_netstack_config_tx = {
    .lwip = {
        .init_fn = ring_link_tx_netstack_init_fn,
        .input_fn = ring_link_tx_netstack_input_fn
    }
};

static const esp_netif_inherent_config_t netif_inherent_config = {
    .flags = ESP_NETIF_FLAG_AUTOUP,
    ESP_COMPILER_DESIGNATED_INIT_AGGREGATE_TYPE_EMPTY(mac)
    ESP_COMPILER_DESIGNATED_INIT_AGGREGATE_TYPE_EMPTY(ip_info)
    .get_ip_event = 0,
    .lost_ip_event = 0,
    .if_key = "ring_link_tx",
    .if_desc = "ring-link-tx if",
    .route_prio = 15,
    .bridge_info = NULL
};


static const esp_netif_config_t netif_config_ = {
    .base = &netif_inherent_config,
    .driver = NULL,
    .stack = &netif_netstack_config_tx,
};

esp_netif_t *get_ring_link_tx_netif(void){
    return ring_link_tx_netif;
}

static esp_err_t esp_netif_ring_link_driver_transmit(void *h, void *buffer, size_t len)
{
    static uint32_t packet_count = 0;
    packet_count++;

    if (buffer == NULL) {
        ESP_LOGI(TAG, "buffer is NULL");
        return ESP_OK;
    }
    ring_link_payload_t p = {
        .id = s_id_counter_tx ++,
        .ttl = RING_LINK_PAYLOAD_TTL,
        .src_id = config_get_id(),
        .dst_id = CONFIG_ID_ANY,
        .buffer_type = RING_LINK_PAYLOAD_TYPE_ESP_NETIF,
        .len = len,
    };
    if (len > RING_LINK_PAYLOAD_BUFFER_SIZE) {
        ESP_LOGE(TAG, "Buffer length exceeds maximum allowed size.");
        return ESP_ERR_INVALID_SIZE;
    }
    memcpy(p.buffer, buffer, len);

    ESP_LOGI(TAG, "[%" PRIu32 "] Transmitting payload - id: %d, src: %d, dst: %d", 
             packet_count, p.id, p.src_id, p.dst_id);

    return send_payload(&p);
}

static esp_err_t ring_link_tx_driver_post_attach(esp_netif_t * esp_netif, void * args)
{
    ESP_LOGI(TAG, "Calling esp_netif_ring_link_post_attach_start(esp_netif_t * esp_netif, void *args)");
    ring_link_netif_driver_t driver = (ring_link_netif_driver_t) args;
    driver->base.netif = esp_netif;
    esp_netif_driver_ifconfig_t driver_ifconfig = {
        .handle = driver,
        .transmit = esp_netif_ring_link_driver_transmit,
        .transmit_wrap = NULL,
        .driver_free_rx_buffer = NULL,
    };

    ESP_ERROR_CHECK(esp_netif_set_driver_config(esp_netif, &driver_ifconfig));

    return ESP_OK;
}

static void ring_link_tx_default_handler(void *arg, esp_event_base_t base, int32_t event_id, void *data)
{
    ESP_LOGI(TAG, "Calling ring_link_tx_default_handler");
    esp_netif_action_got_ip(ring_link_tx_netif, base, event_id, data);
}


static void ring_link_tx_default_action_start(void *arg, esp_event_base_t base, int32_t event_id, void *data)
{
    ESP_LOGI(TAG, "Calling ring_link_tx_default_action_start");
    const esp_netif_ip_info_t ip_info = config_get_tx_ip_info();

    ESP_ERROR_CHECK(esp_netif_set_ip_info(ring_link_tx_netif, &ip_info));

    esp_netif_action_start(ring_link_tx_netif, base, event_id, data);
    ESP_ERROR_CHECK(esp_netif_set_default_netif(ring_link_tx_netif));
}

esp_err_t ring_link_tx_netif_init_(void)
{
    ESP_LOGI(TAG, "Calling ring_link_tx_netif_init");

    ring_link_tx_netif = ring_link_netif_new(&netif_config_);
    
    ESP_ERROR_CHECK(ring_link_netif_esp_netif_attach(ring_link_tx_netif, ring_link_tx_driver_post_attach));

    uint8_t mac[6];
    esp_err_t ret = esp_read_mac(mac, ESP_MAC_WIFI_SOFTAP);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Error getting MAC address");
        return ret;
    }
    esp_netif_set_mac(ring_link_tx_netif, mac);

    ESP_ERROR_CHECK(esp_event_handler_instance_register(RING_LINK_TX_EVENT, RING_LINK_EVENT_START, ring_link_tx_default_action_start, NULL, NULL));
    ESP_ERROR_CHECK(esp_event_post(RING_LINK_TX_EVENT, RING_LINK_EVENT_START, NULL, 0, portMAX_DELAY));
    return ESP_OK;
}
// ============

// === netif RX ===
#include "ring_link_netif_rx.h"

// static const char* TAG = "==> ring_link_netif_rx";

ESP_EVENT_DEFINE_BASE(RING_LINK_RX_EVENT);

static esp_netif_t *ring_link_rx_netif = NULL;
static ring_link_payload_id_t s_id_counter_rx = 0;

esp_netif_recv_ret_t ring_link_rx_netstack_lwip_input_fn(void *h, void *buffer, size_t len, void *l2_buff)
{
    struct netif *netif = h;
    err_t result;
    if (unlikely(!buffer || !netif_is_up(netif))) {
        if (l2_buff) {
            esp_netif_free_rx_buffer(netif->state, l2_buff);
        }
        return ESP_NETIF_OPTIONAL_RETURN_CODE(ESP_FAIL);
    }
    // p = pbuf_alloc(PBUF_RAW, len, PBUF_RAM);
    // memcpy(p->payload, buffer, len);
    struct pbuf *p = (struct pbuf *)buffer;


    result = netif->input(p, netif);

    if (unlikely(result != ERR_OK)) {
        ESP_LOGE("ring_link", "netif->input error: %d", result);
        return ESP_NETIF_OPTIONAL_RETURN_CODE(ESP_FAIL);
    }

    return ESP_NETIF_OPTIONAL_RETURN_CODE(ESP_OK);
}

err_t ring_link_rx_netstack_lwip_init_fn(struct netif *netif)
{
    LWIP_ASSERT("netif != NULL", (netif != NULL));
    netif->name[0]= 'r';
    netif->name[1] = 'x';
    netif->output = output_function;
    netif->linkoutput = linkoutput_function;
    netif->mtu = RING_LINK_PAYLOAD_BUFFER_SIZE;
    netif->hwaddr_len = ETH_HWADDR_LEN;
    netif->flags = NETIF_FLAG_BROADCAST | NETIF_FLAG_LINK_UP;
    return ERR_OK;
}

static const esp_netif_inherent_config_t netif_inherent_config_rx = {
    .flags = ESP_NETIF_FLAG_AUTOUP,
    ESP_COMPILER_DESIGNATED_INIT_AGGREGATE_TYPE_EMPTY(mac)
    ESP_COMPILER_DESIGNATED_INIT_AGGREGATE_TYPE_EMPTY(ip_info)
    .get_ip_event = 0,
    .lost_ip_event = 0,
    .if_key = "ring_link_rx",
    .if_desc = "ring-link-rx if",
    .route_prio = 15,
    .bridge_info = NULL
};

static const struct esp_netif_netstack_config netif_netstack_config = {
    .lwip = {
        .init_fn = ring_link_rx_netstack_lwip_init_fn,
        .input_fn = ring_link_rx_netstack_lwip_input_fn
    }
};


static const esp_netif_config_t netif_config = {
    .base = &netif_inherent_config_rx,
    .driver = NULL,
    .stack = &netif_netstack_config,
};

esp_err_t ring_link_rx_netif_receive(ring_link_payload_t *p)
{
    struct pbuf *q;
    struct ip_hdr *ip_header;
    esp_err_t error;
    // int64_t ring_link_rx_netif_receive_time_init = esp_timer_get_time();

    if (p->len <= 0 || p->len < IP_HLEN) {
        ESP_LOGW(TAG, "Discarding invalid payload.");
        return ESP_OK;
    }

    ip_header = (struct ip_hdr *)p->buffer;

    if (!((IPH_V(ip_header) == 4) || (IPH_V(ip_header) == 6))) {
        ESP_LOGW(TAG, "Discarding non-IP payload.");
        return ESP_OK;
    }

    ip4_addr_t src_ip;
    src_ip.addr = ip_header->src.addr;

    // Convert IP address to host format (little-endian if needed)
    uint32_t addr_ = ntohl(src_ip.addr); // Convert address to host format

    // Extract octets
    uint8_t octet1_ = (addr_ >> 24) & 0xFF; // First octet
    uint8_t octet2_ = (addr_ >> 16) & 0xFF; // Second octet

    // Print IP address for debugging
    ESP_LOGI(TAG, "SRC IP: %d.%d.%d.%d", 
        octet1_, octet2_, (int)((addr_ >> 8) & 0xFF), (int)(addr_ & 0xFF));

    // Print destination IP address
    ip4_addr_t dest_ip;
    dest_ip.addr = ip_header->dest.addr;

    // Convert IP address to host format (little-endian if needed)
    uint32_t addr = ntohl(dest_ip.addr); // Convert address to host format

    // Extract octets
    uint8_t octet1 = (addr >> 24) & 0xFF; // First octet
    uint8_t octet2 = (addr >> 16) & 0xFF; // Second octet

    // Print IP address for debugging
    ESP_LOGI(TAG, "Destination IP: %d.%d.%d.%d", 
        octet1, octet2, (int)((addr >> 8) & 0xFF), (int)(addr & 0xFF));

    // Filter by subnet 192.170.x.x
    if (octet1 != 192 || octet2 != 170) {
        ESP_LOGW(TAG, "Discarding packet not in 192.170.x.x subnet.");
        return ESP_OK;
    }

    // Total IP packet size from header
    u16_t iphdr_len = lwip_ntohs(IPH_LEN(ip_header));

    // Validate that payload size is sufficient
    if (iphdr_len > p->len) {
        ESP_LOGW(TAG, "Payload size (%d) is smaller than IP length (%d). Discarding packet.", p->len, iphdr_len);
        return ESP_OK;
    }

    // Allocate pbuf with required size
    // q = pbuf_alloc(PBUF_TRANSPORT, iphdr_len, PBUF_POOL);
    q = esp_pbuf_allocate(ring_link_rx_netif, p->buffer, iphdr_len, p->buffer);
    if (q == NULL) {
        esp_netif_free_rx_buffer(ring_link_rx_netif, p->buffer);
        return ESP_FAIL;
    }
    // if (q == NULL) {
    //     ESP_LOGW(TAG, "Failed to allocate pbuf.");
    //     return ESP_FAIL;
    // }

    // memcpy(q->payload, p->buffer, iphdr_len);
    // q->next = NULL;
    // q->len = iphdr_len;
    // q->tot_len = iphdr_len;
    // ip4_debug_print(q);

    // Pasar a esp_netif (como antes)
    error = esp_netif_receive(ring_link_rx_netif, q, q->tot_len, NULL);
    if (error != ESP_OK) {
        ESP_LOGW(TAG, "process_thread_receive failed.");
        pbuf_free(q);
    }
    // int64_t ring_link_rx_netif_receive_time_fin = esp_timer_get_time();
    // int64_t total_cycle_us = ring_link_rx_netif_receive_time_fin-ring_link_rx_netif_receive_time_init;
    // printf("[ring_link_rx_netif_receive] payload_id=%d tomó %lld us (%.2f ms)\n", p->id, total_cycle_us, total_cycle_us / 1000.0);


    return error;
    }

static esp_err_t ring_link_rx_driver_transmit(void *h, void *buffer, size_t len)
{
    ESP_LOGW(TAG, "This netif should not transmit but ring_link_rx_driver_transmit was called");
    return ESP_OK;
}

static esp_err_t ring_link_rx_driver_post_attach(esp_netif_t * esp_netif, void * args)
{
    ESP_LOGI(TAG, "Calling esp_netif_ring_link_post_attach_start(esp_netif_t * esp_netif, void *args)");
    ring_link_netif_driver_t driver = (ring_link_netif_driver_t) args;
    driver->base.netif = esp_netif;
    esp_netif_driver_ifconfig_t driver_ifconfig = {
        .handle = driver,
        .transmit = esp_netif_ring_link_driver_transmit,
        .driver_free_rx_buffer = NULL,
    };

    ESP_ERROR_CHECK(esp_netif_set_driver_config(esp_netif, &driver_ifconfig));
    return ESP_OK;
}

static void ring_link_rx_default_handler(void *arg, esp_event_base_t base, int32_t event_id, void *data)
{
    ESP_LOGI(TAG, "Calling ring_link_rx_default_handler");
    esp_netif_action_got_ip(ring_link_rx_netif, base, event_id, data);
}

static void ring_link_rx_default_action_start(void *arg, esp_event_base_t base, int32_t event_id, void *data)
{
    ESP_LOGI(TAG, "Calling ring_link_rx_default_action_start");

    const esp_netif_ip_info_t ip_info = config_get_rx_ip_info();
    
    ESP_ERROR_CHECK(esp_netif_set_ip_info(ring_link_rx_netif, &ip_info));

    uint8_t mac[6] = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55};
    esp_netif_set_mac(ring_link_rx_netif, mac);
    esp_netif_action_start(ring_link_rx_netif, base, event_id, data);
}


esp_err_t ring_link_rx_netif_init_(void)
{
    ESP_LOGI(TAG, "Calling ring_link_rx_netif_init");
    ring_link_rx_netif = ring_link_netif_new(&netif_config);

    ESP_ERROR_CHECK(ring_link_netif_esp_netif_attach(ring_link_rx_netif, ring_link_rx_driver_post_attach));

    ESP_ERROR_CHECK(esp_event_handler_instance_register(RING_LINK_RX_EVENT, RING_LINK_EVENT_START, ring_link_rx_default_action_start, NULL, NULL));
    ESP_ERROR_CHECK(esp_event_post(RING_LINK_RX_EVENT, RING_LINK_EVENT_START, NULL, 0, portMAX_DELAY));
    return ESP_OK;
}

// ================

static esp_err_t ring_link_netif_handler(ring_link_payload_t *p)
{
    if (ring_link_payload_is_for_device(p)) {
        ESP_LOGI(TAG, "[ESP_NETIF] Recibiendo paquete para mí (id=%d)", p->id);
        return ring_link_rx_netif_receive(p);  // tu función real de inyección en stack
    } else {
        ESP_LOGW(TAG, "[ESP_NETIF] Descarta paquete no dirigido a mí. id=%i", p->id);
        return ESP_OK;
    }
}


static esp_err_t process_payload(ring_link_payload_t *p) {
    QueueHandle_t *specific_queue = NULL;

    if (ring_link_payload_is_internal(p)) {
        ESP_LOGD(TAG, "Payload encolado a internal_queue");
        specific_queue = &internal_queue;
    } else if (ring_link_payload_is_esp_netif(p)) {
        ESP_LOGD(TAG, "Payload encolado a esp_netif_queue");
        specific_queue = &esp_netif_queue;
    } else {
        ESP_LOGE(TAG, "Tipo de payload desconocido: 0x%02x", p->buffer_type);
        return ESP_FAIL;
    }

    if (!specific_queue || !(*specific_queue)) {
        ESP_LOGE(TAG, "Cola no inicializada");
        return ESP_FAIL;
    }

    if (xQueueSend(*specific_queue, &p, pdMS_TO_TICKS(100)) != pdTRUE) {
        ESP_LOGW(TAG, "Cola llena, se descarta el payload");
        return ESP_FAIL;
    }

    return ESP_OK;
}

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
            if(spi_to_spi_id==0){
                spi_to_spi_id = msg.payload->id;
            }
            // dump_payload(msg.payload);
            process_payload(msg.payload);

            esp_err_t ret = spi_slave_queue_trans(SPI_RECEIVER_HOST, msg.trans, 100);
            if (ret != ESP_OK) {
                ESP_LOGW(TAG, "No se pudo reencolar la transacción SPI");
            }
        }

    }
}

// === Enviar un payload personalizado desde el master ===

static esp_err_t ring_link_internal_handler(ring_link_payload_t *p)
{
    if (ring_link_payload_is_broadcast(p)) {

        if (ring_link_payload_is_from_device(p)){

            ESP_LOGI(TAG, "[INTERNAL] Broadcast retornado al origen (id=%d)", p->id);

            for (int i = 0; i < MAX_BROADCASTS_IN_FLIGHT; i++) {
                if (broadcast_registry[i].active && broadcast_registry[i].id == p->id) {
                    if (broadcast_registry[i].task) {
                        xTaskNotifyGive(broadcast_registry[i].task);
                    }
                    broadcast_registry[i].active = false;  // Limpiar entrada
                    break;
                }
            }

        }else{
        ESP_LOGI(TAG, "[INTERNAL] Broadcast reenviado.");

        return send_payload(p);  // Reenvía al siguiente nodo

        }

        return ESP_OK;
    }
    else if (ring_link_payload_is_for_device(p)) {
        ESP_LOGI(TAG, "[INTERNAL] Payload dirigido a mí, sin acción.");
        return ESP_OK;
    }
    else {
        if (p->ttl == 0) {
            ESP_LOGW(TAG, "[INTERNAL] TTL agotado. Descarta mensaje id=%d, src=%d → dst=%d",
                     p->id, p->src_id, p->dst_id);
            return ESP_OK;
        }

        p->ttl--;  // <--- Acá se descuenta el TTL antes de reenviar
        ESP_LOGI(TAG, "[INTERNAL] Reenviando payload por SPI. TTL ahora = %d", p->ttl);
        return send_payload(p);  // Reenvía al siguiente nodo
    }
}


void internal_queue_task(void *arg) {
    ring_link_payload_t *p;

    while (1) {
        if (xQueueReceive(internal_queue, &p, portMAX_DELAY)) {
            ESP_LOGI(TAG, "[INTERNAL] Procesando mensaje id=%d", p->id);
            esp_err_t rc = ring_link_internal_handler(p);

            if (rc != ESP_OK) {
                ESP_LOGE(TAG, "[INTERNAL] Error procesando mensaje (id=%d)", p->id);
            }

            // Si usás memoria dinámica en el futuro, liberás acá
            // free(p);
        }
    }
}

void esp_netif_queue_task(void *arg) {
    ring_link_payload_t *p;

    while (1) {
        if (xQueueReceive(esp_netif_queue, &p, portMAX_DELAY)) {
            esp_err_t rc = ring_link_netif_handler(p);

            if (rc != ESP_OK) {
                ESP_LOGE(TAG, "[ESP_NETIF] Error procesando paquete (id=%d)", p->id);
            }

            // Si usaras malloc más adelante, liberarías p acá
            // free(p);
        }
    }
}

// === Master ===

static void spi_master_post_cb(spi_transaction_t *trans) {
    // callback una vez enviada la transaccion por SPI
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
        .cs_ena_posttrans = 1,
        .queue_size = SPI_QUEUE_SIZE,
        .post_cb = spi_master_post_cb,     // <-- acá va el callback
    };

    ESP_ERROR_CHECK(spi_bus_initialize(SPI_SENDER_HOST, &buscfg, SPI_DMA_CH_AUTO));
    ESP_ERROR_CHECK(spi_bus_add_device(SPI_SENDER_HOST, &devcfg, &spi_handle));
    assert(spi_handle != NULL);

}

esp_err_t send_custom_payload(
    ring_link_payload_buffer_type_t buffer_type,
    const void *data,
    uint8_t len,
    ring_link_payload_id_t id,
    config_id_t src_id,
    config_id_t dst_id,
    uint8_t ttl)
{
    if (len > RING_LINK_PAYLOAD_BUFFER_SIZE) {
        ESP_LOGE(TAG, "Tamaño de mensaje demasiado grande (%d > %d)", len, RING_LINK_PAYLOAD_BUFFER_SIZE);
        return ESP_ERR_INVALID_SIZE;
    }

    ring_link_payload_t payload = {
        .id = id,
        .buffer_type = buffer_type,
        .len = len,
        .ttl = ttl,
        .src_id = src_id,
        .dst_id = dst_id
    };

    memcpy(payload.buffer, data, len);

    return send_payload(&payload);  // usa la función que ya tenés definida
}

// === MAIN ===
void app_main(void) {
    ESP_ERROR_CHECK(init_nvs());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    ESP_ERROR_CHECK(esp_netif_init());
    config_setup();

    rx_queue = xQueueCreate(SPI_QUEUE_SIZE, sizeof(spi_payload_msg_t));
    assert(rx_queue != NULL);  // ← esto es crítico para evitar uso de puntero NULL

    internal_queue = xQueueCreate(20, sizeof(ring_link_payload_t *));
    esp_netif_queue = xQueueCreate(40, sizeof(ring_link_payload_t *));
    assert(internal_queue && esp_netif_queue);
    spi_mutex = xSemaphoreCreateMutex();
    assert(spi_mutex != NULL);


    spi_master_init();
    ESP_ERROR_CHECK(ring_link_rx_netif_init_());
    ESP_ERROR_CHECK(ring_link_tx_netif_init_());

    xTaskCreate(spi_slave_task, "spi_slave_task", 4096*2, NULL, 13, NULL);
    xTaskCreate(process_task, "process_task", 4096*2, NULL, 14, NULL);
    xTaskCreate(internal_queue_task, "internal_queue_task", 2048, NULL, 9, NULL);
    xTaskCreate(esp_netif_queue_task, "esp_netif_queue_task", 4096, NULL, 9, NULL);


    ESP_ERROR_CHECK(wifi_init());
    ESP_ERROR_CHECK(wifi_netif_init());
    // print_route_table();

    // init_broadcast_registry();


}
