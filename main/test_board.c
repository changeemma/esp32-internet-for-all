#include "test_board.h"

const char *BOARD_TEST_TAG = "==> BOARD TEST";

void send_test_payload(){
    ESP_LOGI(BOARD_TEST_TAG, "sending %s", SPI_TEST_PAYLOAD);
    spi_lowlevel_transmit(SPI_TEST_PAYLOAD, sizeof(SPI_TEST_PAYLOAD));
}

void receive_test_payload(){
    char test_buffer[sizeof(SPI_TEST_PAYLOAD)];
    size_t received;

    received = spi_lowlevel_receive(test_buffer, sizeof(SPI_TEST_PAYLOAD));
    printf("%s, %d\n", test_buffer, received);
    assert(strcmp(test_buffer, SPI_TEST_PAYLOAD) == 0);
    ESP_LOGI(BOARD_TEST_TAG, "received %s", test_buffer);
}

esp_err_t test_spi(bool activate) {
    if(activate==1){
            
        ESP_LOGI(BOARD_TEST_TAG, "========== TEST SPI ==========");

        if (device_config_get_mode() == DEVICE_MODE_ACCESS_POINT) {
            vTaskDelay(1000/portTICK_PERIOD_MS); // delay transmision to wait for others
            send_test_payload();
            receive_test_payload();
        } else
        {
            receive_test_payload();
            send_test_payload();
        }

        ESP_LOGI(BOARD_TEST_TAG, "========== TEST SPI ==========");
    }

    ESP_LOGI(BOARD_TEST_TAG, "========== TEST SPI ==========");
    return ESP_OK;
}
