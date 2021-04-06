#include "esp_system.h"
#include "esp_log.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <errno.h>

#include "nvs.h"
#include "nvs_flash.h"

#include "wifi_state.h"
#include "state_core.h"
#include "net_state.h"
#include "global_defines.h"

/*********************************************************
*                  STATIC VARIABLES
**********************************************************/
static const char TAG[] = "NET_STATE";
static func_ptr func_table[net_state_len];

/*********************************************************
*                     FUNCTIONS
**********************************************************/
static void state_wait_for_wifi() {
  // do nothing
}

static void state_wait_for_provisions() {
  // do nothing
}

static void state_upload_data() {
}

// returns the next state
static func_ptr update_state(state_t * curr_state, state_event_t event, func_ptr old_func){
  if (*curr_state == net_waiting_wifi){
    if (event == wifi_connect){
        ESP_LOGI(TAG, "New state net_wait_prov");
        *curr_state = net_waiting_prov;
        return state_wait_for_provisions;
    }
  }

  if (*curr_state == net_waiting_prov ){
    if (event == wifi_disconnect){
        ESP_LOGI(TAG, "New state net_waiting_wifi");
        *curr_state = net_waiting_prov;
        return state_wait_for_provisions;
    }
  }

  // Don't change state
  return old_func;
}

static void net_state(void* v) {
    func_ptr fptr = state_wait_for_wifi;
    state_event_t new_event;
    state_t state;
    BaseType_t xStatus; 

    for (;;) {
      //execute current function
      fptr();

      //get event
      new_event = INVALID_EVENT;
      xStatus = xQueueReceive(events_net_q, &new_event, RTOS_DONT_WAIT);
      if (xStatus == pdFALSE){
        // no new event, sleep for 1 second
        vTaskDelay( 1000 / portTICK_PERIOD_MS);
        continue;
      }

      // rxed an event, see if we need to change state
      if (new_event != INVALID_EVENT){
        ESP_LOGI(TAG, "Rxed valid event == %d", new_event);
        fptr = update_state(&state, new_event, fptr);
      }else{
        ESP_LOGE(TAG,"New event not set!");
        ASSERT(0);
      } 

      //reset new_event
      new_event = INVALID_EVENT;
    }
}

static void init_fnc_table(){
  func_table[net_waiting_wifi] = state_wait_for_wifi;
  func_table[net_waiting_prov] = state_wait_for_provisions;
  func_table[net_running] = state_upload_data;
}

void net_state_spawner() {
    BaseType_t rc;

    init_fnc_table();
    rc = xTaskCreate(net_state,
                     "net_state",
                     4096,
                     NULL,
                     4,
                     NULL);

    if (rc != pdPASS) {
        assert(0);
    }
}
