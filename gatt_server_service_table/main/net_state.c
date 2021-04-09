#include "esp_log.h"
#include "esp_system.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include <errno.h>

#include "nvs.h"
#include "nvs_flash.h"

#include "global_defines.h"
#include "net_state.h"
#include "state_core.h"
#include "wifi_state.h"

/*********************************************************
*                  STATIC VARIABLES
**********************************************************/
static const char TAG[] = "NET_STATE";
static SemaphoreHandle_t  net_state_mutex;
static bool net_state;

/**********************************************************
*                 FORWARD DECLARATIONS 
**********************************************************/

/*********************************************************
*                   STATE FUNCTIONS
**********************************************************/
static void state_wait_for_wifi_func() {
    ESP_LOGI(TAG, "Entering wait_for_wifi state");
    // do nothing
    set_net_state(false);;
}

static void state_wait_for_provisions_func() {
    ESP_LOGI(TAG, "Entering wait_for_prov state");
    // do nothing
    set_net_state(true);
}

static void state_upload_data_func() {
}

// Returns the next state
static void next_state_func(state_t* curr_state, state_event_t event) {
    if (!curr_state) {
        ESP_LOGE(TAG, "ARG= NULL!");
        ASSERT(0);
    }

    if (*curr_state == net_waiting_wifi) {
        if (event == wifi_connect) {
            ESP_LOGI(TAG, "Old State: net_waiting_wifi, Next: net_waiting_prov");
            *curr_state = net_waiting_prov;
            return;
        }
    }

    if (*curr_state == net_waiting_prov) {
        if (event == wifi_disconnect) {
            ESP_LOGI(TAG, "Old State: net_waitin_prov, Next: net_waiting_wifi");
            *curr_state = net_waiting_wifi;
        }
    }

    // Stay in the same state
}

char* event_print_func(state_event_t event) {
    switch (event) {
    case (wifi_disconnect):
        return "wifi_disconnect";
        break;
    case (wifi_connect):
        return "wifi_connect";
        break;
    }
    // event not targeted at this state machine
    return NULL;
}

// Returns the state function, given a state
static state_array_s get_state_func(state_t current_state) {
    static state_array_s  func_table[net_state_len] = {
      //{      (state function)           , looper time },  
        { state_wait_for_wifi_func        , portMAX_DELAY            },
        { state_wait_for_provisions_func  , 1000/portTICK_PERIOD_MS  },
        { state_upload_data_func          , 2000/portTICK_PERIOD_MS  },
    };

    if (current_state >= net_state_len) {
        ESP_LOGE(TAG, "Current state out of bounds!");
        ASSERT(0);
    }

    return func_table[current_state];
}

static state_event_t get_event_func(uint32_t timeout) {
    state_event_t new_event = INVALID_EVENT;
    xQueueReceive(events_net_q, &new_event, timeout);
    return new_event;
}

/*********************************************************
*                NON-STATE  FUNCTIONS
**********************************************************/
void set_net_state(bool state){
    if (pdTRUE != xSemaphoreTake(net_state_mutex, NET_SATE_MUTEX_WAIT)) {
        ESP_LOGE(TAG, "FAILED TO TAKE NET_SATE_MUTEX !");
        ASSERT(0);
    }
    net_state = state;
    xSemaphoreGive(net_state_mutex);
}

bool get_net_state(){
    if (pdTRUE != xSemaphoreTake(net_state_mutex, NET_SATE_MUTEX_WAIT)) {
        ESP_LOGE(TAG, "FAILED TO TAKE NET_SATE_MUTEX !");
        ASSERT(0);
    }
    bool ret = net_state;
    xSemaphoreGive(net_state_mutex);
 
    return ret;
}

static void net_state_init_freertos_objects() {
    net_state_mutex = xSemaphoreCreateMutex();
    
    // make sure we init all the rtos objects
    ASSERT(net_state_mutex);
}

static bool event_filter_func(state_event_t event) {
  return false;
}

state_init_s * get_net_state_handle(){
    static state_init_s net_state = {
        .next_state        = next_state_func,
        .get_event         = get_event_func,
        .translator        = get_state_func,
        .event_print       = event_print_func,
        .state_name_string = "net_state",
        .filter_event      = event_filter_func,
    };
  return &(net_state);
}


void net_state_spawner() {
    net_state_init_freertos_objects();

    // State the state machine
    start_new_state_machine( get_net_state_handle() );
}
