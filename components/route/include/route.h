#ifndef __ROUTE_H
#define __ROUTE_H


#include <stdio.h>
#include <string.h>
#include "esp_wifi.h"

#include "ring_link_netif.h"

#define MAX_NUMBER_ROUTES 10

typedef enum {
    SPI_GATEWAY = 0,
    WIFI_GATEWAY = 1
} gateway_t;

esp_err_t add_route(char * ip, gateway_t gw, char * netmask, char * key);
esp_err_t rm_route(char * ip, char * netmask);

#endif
