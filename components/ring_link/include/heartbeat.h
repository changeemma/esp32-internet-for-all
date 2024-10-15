#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <esp_timer.h>
#include <esp_log.h>

#include "ring_link_lowlevel.h"
#include "ring_link_internal.h"
#include "ring_link_netif.h"


#ifdef __cplusplus
extern "C" {
#endif

#define HEARTBEAT_PAYLOAD "HEARTBEAT..."
#define HEARTBEAT_INTERVAL_SEC 5
#define HEARTBEAT_INTERVAL_USEC (HEARTBEAT_INTERVAL_SEC*1000000)
#define HEARTBEAT_MAX_FAILURES 3  // Number of consecutive failures before considering a board as "out"


void init_heartbeat(void);


#ifdef __cplusplus
}
#endif
