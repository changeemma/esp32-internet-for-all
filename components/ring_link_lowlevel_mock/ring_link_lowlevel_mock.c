#include "ring_link_lowlevel_mock.h"
#include "esp_log.h"
#include <stdio.h>

static const char *TAG = "ring_link_lowlevel_mock";

// Variables globales para contar las llamadas
esp_err_t mock_init_test(void) {
    ESP_LOGI(TAG, "mock_init_test called");
    printf("mock_init_test called\n");
    init_called++;
    return ESP_OK;
}

esp_err_t mock_transmit_test(void* data, size_t length) {
    ESP_LOGI(TAG, "mock_transmit_test called with length %zu", length);
    printf("mock_transmit_test called with length %zu\n", length);
    transmit_called++;
    return ESP_OK;
}

esp_err_t mock_receive_test(void* data, size_t length) {
    ESP_LOGI(TAG, "mock_receive_test called with length %zu", length);
    printf("mock_receive_test called with length %zu\n", length);
    receive_called++;
    return ESP_OK;
}

// Función auxiliar para reiniciar los contadores
void reset_mock_counters(void) {
    init_called = 0;
    transmit_called = 0;
    receive_called = 0;
    ESP_LOGI(TAG, "Mock counters reset");
    printf("Mock counters reset\n");
}