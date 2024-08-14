#pragma once

// C libraries
#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "esp_check.h"


#ifdef __cplusplus
extern "C" {
#endif

#define UART_BUFFER_SIZE 10


esp_err_t uart_init(void);
esp_err_t uart_transmit(void *p, size_t len);
esp_err_t uart_receive(void *p, size_t len);


#ifdef __cplusplus
}
#endif

