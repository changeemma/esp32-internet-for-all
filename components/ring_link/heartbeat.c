#include "heartbeat.h"


static int heartbeat_id = 0;
static bool heartbeat_received = false;
static esp_timer_handle_t heartbeat_timer;
static esp_timer_handle_t check_timer;
static int failure_count = 0;

static const char *TAG = "==> heartbeat";

static void offline_board_callback(){
    ESP_LOGE(TAG, "There is an OFFLINE board. Callback invoked.");
}

static void online_board_callback(){
    ESP_LOGI(TAG, "The node is back ONLINE. Callback invoked.");
}

static void send_heartbeat() {
    heartbeat_id++;
    const char msg[] = "HEARTBEAT...";
    heartbeat_received = broadcast_to_siblings_heartbeat(msg, sizeof(msg));
    ESP_LOGD(TAG, "Sent heartbeat %d", heartbeat_id);
}

static void check_heartbeat() {
    if (!heartbeat_received) {
        failure_count++;
        ESP_LOGE(TAG, "Heartbeat %d not received. Failure #%d", heartbeat_id, failure_count);
        if (failure_count >= MAX_FAILURES) {
            ESP_LOGE(TAG, "Maximum failures reached. Board considered out of service.");
            offline_board_callback();
        }
    } else
    {
        ESP_LOGD(TAG, "The node is ONLINE.");
        if (failure_count > 0) {
            failure_count = 0;  // Reset the counter if heartbeat is received and it was failing before
            online_board_callback();
        }
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