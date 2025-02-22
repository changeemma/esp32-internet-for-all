#include "heartbeat.h"

static const char *TAG = "==> heartbeat";

static bool node_online = false;
static int heartbeat_id = 0;
static int failure_count = 0;
static TaskHandle_t s_heartbeat_task = NULL;


static void online_board_callback(){
    ESP_LOGI(TAG, "online_board_callback invoked.");
}

static void offline_board_callback(){
    ESP_LOGE(TAG, "offline_board_callback invoked.");
}

static void heartbeat_callback() {
    heartbeat_id++;
    if (broadcast_to_siblings(HEARTBEAT_PAYLOAD, sizeof(HEARTBEAT_PAYLOAD))) 
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

esp_err_t heartbeat_init(void) {
    esp_timer_handle_t heartbeat;

    esp_timer_create_args_t heartbeat_args = {
        .callback = &heartbeat_callback,
        .name = "heartbeat"
    };
    ESP_ERROR_CHECK(esp_timer_create(&heartbeat_args, &heartbeat));
    ESP_ERROR_CHECK(esp_timer_start_periodic(heartbeat, HEARTBEAT_INTERVAL_USEC));
    return ESP_OK;
}