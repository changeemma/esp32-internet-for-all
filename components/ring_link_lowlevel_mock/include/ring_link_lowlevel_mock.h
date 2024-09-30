#ifndef __RING_LINK_LOWLEVEL_MOCK_H
#define __RING_LINK_LOWLEVEL_MOCK_H

#include "esp_log.h"
#include "esp_check.h"

static int init_called = 0;
static int transmit_called = 0;
static int receive_called = 0;
esp_err_t mock_init_test(void);
esp_err_t mock_transmit_test(void* data, size_t length);
esp_err_t mock_receive_test(void* data, size_t length);
void reset_mock_counters(void);
#endif
