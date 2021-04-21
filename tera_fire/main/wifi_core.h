#pragma once

#include "esp_err.h"
#include "esp_netif.h"
#include "stdint.h"

/**********************************************************
*                                                 DEFINES *
**********************************************************/
#define CONFIG_EXAMPLE_CONNECT_WIFI
#define CONFIG_EXAMPLE_WIFI_SSID "test-wifi"

#ifdef CONFIG_EXAMPLE_CONNECT_ETHERNET
#define EXAMPLE_INTERFACE get_example_netif()
#endif

#ifdef CONFIG_EXAMPLE_CONNECT_WIFI
#define EXAMPLE_INTERFACE get_example_netif()
#endif

#define WIFI_STATE_CONNECTED    (0)
#define WIFI_STATE_DISCONNECTED (1)

#define WIFI_MUTEX_WAIT (5000 / portTICK_PERIOD_MS)
#define WIFI_DOWN       (0)
#define WIFI_UP         (1)

/*********************************************************
*                                       GLOBAL FUNCTIONS *
*********************************************************/
void         wifi_core_connect();
void         wifi_core_stop();
void         wifi_core_init_freertos_objects();
esp_netif_t* get_example_netif(void);
void         init_wifi();
bool         get_wifi_state();
