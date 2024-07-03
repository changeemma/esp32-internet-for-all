#ifndef __TEST_BOARD_H
#define __TEST_BOARD_H

#include "config.h"
#include "physical_spi.h"

#define SPI_TEST_PAYLOAD "hello world 123456789"


esp_err_t test_spi(bool);

#endif