#include "unity.h"
#include "ring_link_lowlevel_mock.h"
#include "ring_link_lowlevel.h"
#include "esp_log.h"

static const char *TAG = "test_ring_link_lowlevel";
extern int init_called;

TEST_CASE("Test ring_link_lowlevel_init", "[ring_link_lowlevel]")
{
    reset_mock_counters();
    esp_err_t result = ring_link_lowlevel_init();
    ESP_LOGI(TAG, "ring_link_lowlevel_init result: %d, init_called: %d", result, init_called);
    TEST_ASSERT_EQUAL(ESP_OK, result);
    TEST_ASSERT_EQUAL_MESSAGE(1, init_called, "mock_init_test should be called exactly once");
}

TEST_CASE("Test ring_link_lowlevel_transmit_payload", "[ring_link_lowlevel]")
{
    reset_mock_counters();
    ring_link_payload_t payload = {0};
    esp_err_t result = ring_link_lowlevel_transmit_payload(&payload);
    ESP_LOGI(TAG, "ring_link_lowlevel_transmit_payload result: %d, transmit_called: %d", result, transmit_called);
    TEST_ASSERT_EQUAL(ESP_OK, result);
    TEST_ASSERT_EQUAL_MESSAGE(1, transmit_called, "mock_transmit_test should be called exactly once");
}

TEST_CASE("Test ring_link_lowlevel_receive_payload", "[ring_link_lowlevel]")
{
    reset_mock_counters();
    ring_link_payload_t payload = {0};
    esp_err_t result = ring_link_lowlevel_receive_payload(&payload);
    ESP_LOGI(TAG, "ring_link_lowlevel_receive_payload result: %d, receive_called: %d", result, receive_called);
    TEST_ASSERT_EQUAL(ESP_OK, result);
    TEST_ASSERT_EQUAL_MESSAGE(1, receive_called, "mock_receive_test should be called exactly once");
}

TEST_CASE("Test ring_link_lowlevel_forward_payload", "[ring_link_lowlevel]")
{
    reset_mock_counters();
    ring_link_payload_t payload = {.ttl = 2, .id = 1, .dst_device_id = 3};
    esp_err_t result = ring_link_lowlevel_forward_payload(&payload);
    ESP_LOGI(TAG, "ring_link_lowlevel_forward_payload result: %d, transmit_called: %d, ttl: %d", result, transmit_called, payload.ttl);
    TEST_ASSERT_EQUAL(ESP_OK, result);
    TEST_ASSERT_EQUAL_MESSAGE(1, transmit_called, "mock_transmit_test should be called exactly once");
    TEST_ASSERT_EQUAL(1, payload.ttl);
}

TEST_CASE("Test ring_link_lowlevel_forward_payload_discard", "[ring_link_lowlevel]")
{
    reset_mock_counters();
    ring_link_payload_t payload = {.ttl = 0, .id = 1, .dst_device_id = 3};
    esp_err_t result = ring_link_lowlevel_forward_payload(&payload);
    ESP_LOGI(TAG, "ring_link_lowlevel_forward_payload_discard result: %d, transmit_called: %d", result, transmit_called);
    TEST_ASSERT_EQUAL(ESP_OK, result);
    TEST_ASSERT_EQUAL_MESSAGE(0, transmit_called, "mock_transmit_test should not be called");
}