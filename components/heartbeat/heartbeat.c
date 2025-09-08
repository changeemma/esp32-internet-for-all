#include "heartbeat.h"

static const char *TAG = "heartbeat";

static bool s_node_online = false;
static size_t s_heartbeat_id = 0;
static size_t s_success_streak = 0;
static size_t s_fail_streak = 0;

static void node_online_callback(){
    ESP_LOGI(TAG, "calling node_online_callback()");
}

static void node_offline_callback(){
    ESP_LOGE(TAG, "calling node_offline_callback()");
}

static void heartbeat_callback() {
    const int64_t start_time = esp_timer_get_time();

    bool success = broadcast_to_siblings(HEARTBEAT_PAYLOAD, sizeof(HEARTBEAT_PAYLOAD));

    const int64_t duration = esp_timer_get_time() - start_time;

    if (success) {
        s_fail_streak = 0;  // reset failure counter
        s_success_streak++;
        //ESP_LOGD(TAG, "Heartbeat %d succeeded.", s_heartbeat_id);
        ESP_LOGI(TAG, "#%d succeeded in %lld µs", s_heartbeat_id, duration);

        if (!s_node_online && (s_success_streak >= HEARTBEAT_SUCCESS_THRESHOLD)) {
            ESP_LOGI(TAG, "Node marked ONLINE after %d successful heartbeats", s_success_streak);
            s_node_online = true;
            node_online_callback();
        }
    } else {
        s_success_streak = 0;
        s_fail_streak++;
        ESP_LOGW(TAG, "#%d failed (streak: %d)", s_heartbeat_id, s_fail_streak);

        if (s_node_online && (s_fail_streak >= HEARTBEAT_FAIL_THRESHOLD)) {
            ESP_LOGE(TAG, "Node marked OFFLINE after %d consecutive failures", s_fail_streak);
            s_node_online = false;
            node_offline_callback();
        }
    }
    s_heartbeat_id++;
}

esp_err_t heartbeat_init(void) {
    if (!config_mode_is(CONFIG_MODE_ACCESS_POINT)) {
        ESP_LOGI(TAG, "TEST_MODE: heartbeat_init: skipping...");
        return ESP_OK;
    }
    ESP_LOGI(TAG, "TEST_MODE: heartbeat_init: initializing...");
    
    esp_timer_handle_t timer_handle;
    esp_timer_create_args_t timer_args = {
        .name                  = HEARTBEAT_TIMER_NAME,
        .callback              = &heartbeat_callback,
        .skip_unhandled_events = true
    };

    ESP_ERROR_CHECK(esp_timer_create(&timer_args, &timer_handle));
    ESP_ERROR_CHECK(esp_timer_start_periodic(timer_handle, HEARTBEAT_PERIOD));

    return ESP_OK;
}