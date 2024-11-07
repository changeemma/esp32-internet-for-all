#include "heartbeat.h"

static const char *TAG = "==> heartbeat";

static bool node_online = false;
static int heartbeat_id = 0;
static int failure_count = 0;
static TaskHandle_t s_heartbeat_task = NULL;


static bool send_heartbeat()
{
    bool result = false;
    ring_link_payload_t p = {
        .id = heartbeat_id,
        .ttl = RING_LINK_PAYLOAD_TTL,
        .src_id = config_get_id(),
        .dst_id = config_get_id(),
        .buffer_type = RING_LINK_PAYLOAD_TYPE_INTERNAL_HEARTBEAT,
        .len = sizeof(HEARTBEAT_PAYLOAD),
        .buffer = HEARTBEAT_PAYLOAD,
    };

    s_heartbeat_task = xTaskGetCurrentTaskHandle();
    if (ring_link_lowlevel_transmit_payload(&p) == ESP_OK)
    {    
        result = ulTaskNotifyTake( pdTRUE, ( TickType_t ) 100 ) == pdTRUE ? true : false;
    }
    s_heartbeat_task = NULL;
    return result;
}

esp_err_t ring_link_heartbeat_handler(ring_link_payload_t *p)
{
    if (ring_link_payload_is_from_device(p))
    {
        ESP_LOGD(TAG, "Heartbeat complete (src=%i,dest=%i,id=%i,ttl=%i).", p->src_id, p->dst_id, p->id, p->ttl);
        xTaskNotifyGive( s_heartbeat_task );
        return ESP_OK;
    }
    else
    {
        return ring_link_lowlevel_forward_payload(p);
    }
}

static void online_board_callback(){
    ESP_LOGI(TAG, "online_board_callback invoked.");
}

static void offline_board_callback(){
    ESP_LOGE(TAG, "offline_board_callback invoked.");
}

static void heartbeat_callback() {
    heartbeat_id++;
    if (send_heartbeat()) 
    {
        failure_count = 0;
        ESP_LOGD(TAG, "Heartbeat %d succeded.", heartbeat_id);
        if (!node_online) {
            ESP_LOGI(TAG, "Node is ONLINE.");
            node_online = true;
            online_board_callback();
        }
    } 
    else
    {
        failure_count++;
        ESP_LOGW(TAG, "Heartbeat %d failed. Failure #%d", heartbeat_id, failure_count);
        if (node_online && (failure_count >= HEARTBEAT_MAX_FAILURES)) {
            ESP_LOGE(TAG, "Maximum failures reached. Node considered OFFLINE.");
            node_online = false;
            offline_board_callback();
        }
    }
}

void init_heartbeat(void) {
    esp_timer_handle_t heartbeat;

    esp_timer_create_args_t heartbeat_args = {
        .callback = &heartbeat_callback,
        .name = "heartbeat"
    };
    ESP_ERROR_CHECK(esp_timer_create(&heartbeat_args, &heartbeat));
    ESP_ERROR_CHECK(esp_timer_start_periodic(heartbeat, HEARTBEAT_INTERVAL_USEC));
}