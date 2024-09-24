#include "heartbeat.h"

#define HEARTBEAT_INTERVAL_SEC 5
#define MAX_FAILURES 5  // Number of consecutive failures before considering a board as "out"

static int heartbeat_id = 0;
static bool heartbeat_received = false;
static esp_timer_handle_t heartbeat_timer;
static esp_timer_handle_t check_timer;
static int failure_count = 0;

static const char *TAG = "==> heartbeat";

void offline_board_callback(int failure_count){
    ESP_LOGE(TAG, "There is an OFFLINE board. Callback invoked. After %d tests.", failure_count);
}

void online_board_callback(){
    ESP_LOGD(TAG, "The node is ONLINE. Callback invoked.");
}

void send_heartbeat() {
    heartbeat_id++;
    const char msg[] = "HEARTBEAT...";
    heartbeat_received = broadcast_to_siblings_heartbeat(msg, sizeof(msg));
    ESP_LOGD(TAG, "Sent heartbeat %d", heartbeat_id);
}

void check_heartbeat() {
    if (!heartbeat_received) {
        failure_count++;
        ESP_LOGW("HEARTBEAT", "Heartbeat %d not received. Failure #%d", heartbeat_id, failure_count);
        if (failure_count >= MAX_FAILURES) {
            ESP_LOGE(TAG, "Maximum failures reached. Board considered out of service.");
            offline_board_callback(failure_count);
        }
    } else {
        failure_count = 0;  // Reset the counter if heartbeat is received
        online_board_callback();
    }
}

void heartbeat_timer_callback(void* arg) {
    send_heartbeat();
    check_heartbeat();
}

void check_timer_callback(void* arg) {
}

void init_heartbeat(void) {
    esp_timer_create_args_t heartbeat_timer_args = {
        .callback = &heartbeat_timer_callback,
        .name = "heartbeat_timer"
    };
    ESP_ERROR_CHECK(esp_timer_create(&heartbeat_timer_args, &heartbeat_timer));
    ESP_ERROR_CHECK(esp_timer_start_periodic(heartbeat_timer, HEARTBEAT_INTERVAL_SEC * 1000000));
}