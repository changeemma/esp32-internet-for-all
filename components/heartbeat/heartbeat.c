#include "heartbeat.h"

static const char *TAG = "==> heartbeat";

static bool s_node_online = false;
static int s_heartbeat_id = 0;
static int s_failure_count = 0;

static void online_board_callback(){
    ESP_LOGI(TAG, "Node is ONLINE.");
    s_node_online = true;
    s_heartbeat_id = 0;
}

static void offline_board_callback(){
    ESP_LOGE(TAG, "Maximum failures reached. Node considered OFFLINE.");
    s_node_online = false;
}

static void heartbeat_callback() {
    bool succeded = broadcast_to_siblings(HEARTBEAT_PAYLOAD, sizeof(HEARTBEAT_PAYLOAD));
    if (succeded) 
    {
        s_failure_count = 0;  // reset counter after each success
        ESP_LOGD(TAG, "Heartbeat %d succeded.", s_heartbeat_id);
        if (!s_node_online) {
            online_board_callback();
        }
    } 
    else
    {
        s_failure_count++;
        ESP_LOGW(TAG, "Heartbeat %d failed. Failure #%d", s_heartbeat_id, s_failure_count);
        if (s_node_online && (s_failure_count >= HEARTBEAT_MAX_FAILURES)) {
            offline_board_callback();
        }
    }
    s_heartbeat_id++;
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