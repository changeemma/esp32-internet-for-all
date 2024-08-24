#include "ring_link.h"
#include "unity.h"

static esp_err_t mock_internal_handler(ring_link_payload_t *p) {
    // Mock implementation
    return ESP_OK;
}

static esp_err_t mock_netif_handler(ring_link_payload_t *p) {
    // Mock implementation
    return ESP_OK;
}

static ring_link_handlers_t original_handlers;

void test_setup(void) {
    // Save the original handlers
    original_handlers = ring_link_handlers;
    // Set up the mocks
    ring_link_handlers.internal_handler = mock_internal_handler;
    ring_link_handlers.netif_handler = mock_netif_handler;
}

void test_teardown(void) {
    // Restore the original handlers
    ring_link_handlers = original_handlers;
}

TEST_CASE("Test process_ring_link_payload", "[ring_link]")
{
    test_setup();

    ring_link_payload_t p;

    // Test INTERNAL payload
    p.buffer_type = RING_LINK_PAYLOAD_TYPE_INTERNAL;
    TEST_ASSERT_EQUAL(ESP_OK, process_ring_link_payload(&p));

    // Test INTERNAL_HEARTBEAT payload
    p.buffer_type = RING_LINK_PAYLOAD_TYPE_INTERNAL_HEARTBEAT;
    TEST_ASSERT_EQUAL(ESP_OK, process_ring_link_payload(&p));

    // Test ESP_NETIF payload
    p.buffer_type = RING_LINK_PAYLOAD_TYPE_ESP_NETIF;
    TEST_ASSERT_EQUAL(ESP_OK, process_ring_link_payload(&p));

    // Test unknown payload type
    p.buffer_type = 99;  // Unknown type
    TEST_ASSERT_EQUAL(ESP_FAIL, process_ring_link_payload(&p));

    test_teardown();
}