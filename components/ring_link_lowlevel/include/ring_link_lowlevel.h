#pragma once


// ESP32 libraries
#include "esp_check.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

#include "ring_link_payload.h"
#include "ring_link_lowlevel_impl.h"
#include "config.h"

#ifdef __cplusplus
extern "C" {
#endif


esp_err_t ring_link_lowlevel_init(void);

esp_err_t ring_link_lowlevel_transmit_payload(ring_link_payload_t *p);

esp_err_t ring_link_lowlevel_receive_payload(ring_link_payload_t *p);

esp_err_t ring_link_lowlevel_forward_payload(ring_link_payload_t *p);


#ifdef __cplusplus
}
#endif

