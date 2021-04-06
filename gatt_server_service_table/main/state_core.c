#include "esp_system.h"
#include <stdio.h>
#include "esp_log.h"
#include <stdlib.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <errno.h>

#include "global_defines.h"
#include "state_core.h"


/**********************************************************
*                   GLOBAL VARIABLES
**********************************************************/
QueueSetHandle_t  events_net_q;

/**********************************************************
*                  STATIC VARIABLES
**********************************************************/
static QueueSetHandle_t  incoming_events_q;
static const char TAG[] = "STATE_CORE";

/**********************************************************
*                 FORWARD DECLARATIONS 
**********************************************************/


/**********************************************************
*                       FUNCTIONS
**********************************************************/


// this emulates DBUS on standard UNIX distros
// the thread reads an incoming value from a queue
// and then multiplexes it out on multiple queues
// state machines on other threads listen into these 
// queues to select their next state
static void dbus(void* v) {
    for (;;) {
      state_event_t rx;
      BaseType_t xStatus;
 
      xStatus = xQueueReceive(incoming_events_q, (void*)&rx, portMAX_DELAY);
      if (xStatus != pdTRUE){
        ESP_LOGE(TAG, "Failed to rx... can't recover..");
        ASSERT(0);
      }

      ESP_LOGI(TAG, "RXed an event! %d", rx);

      xStatus = xQueueSendToBack(events_net_q, (void*)&rx, RTOS_DONT_WAIT);
      if (xStatus != pdTRUE){
        ESP_LOGE(TAG, "Failed to read queue! (net)");
        ASSERT(0);
      }
    }
}

static void state_core_init_freertos_objects() {
    //Reads and Pushes events from state-machines
    incoming_events_q    = xQueueCreate(MAX_QUEUE_DEPTH, sizeof(state_event_t));  // state-machines -> state-core
    events_net_q  = xQueueCreate(MAX_QUEUE_DEPTH, sizeof(state_event_t)); // state-core     -> net-sm

    // make sure nothing is NULL!
    ASSERT(incoming_events_q);
    ASSERT(events_net_q);
}

void state_post_event(state_event_t event){
  BaseType_t xStatus = xQueueSendToBack(incoming_events_q, (void*)&event, RTOS_DONT_WAIT);
  if (xStatus != pdTRUE){
      ESP_LOGE(TAG, "Failed to enqueue to dbus!");
      ASSERT(0);
  }
}

void state_core_spawner() {
    BaseType_t rc;
  
    state_core_init_freertos_objects();
    rc = xTaskCreate(dbus,
                     "dbus",
                     4096,
                     NULL,
                     4,
                     NULL);

    if (rc != pdPASS) {
        ASSERT(0);
    }
}