// C libraries
#include <stdio.h>

// ESP32 libraries
#include "esp_event.h"
#include "esp_check.h"

// Private components
#include "config.h"
#include "physical_spi.h"
#include "spi_netif.h"
#include "spi_rx_netif.h"
#include "spi_tx_netif.h"
#include "udp_spi.h"
#include "wifi.h"
#include "route.h"

#include "nvs.h"

// Test files
#include "test_board.h"
#include "test_integration.h"

#define TEST_ALL true

static const char *TAG = "==> main";
