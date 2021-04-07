#include "esp_log.h"
#include "esp_system.h"
#include <stdio.h>
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
QueueSetHandle_t events_net_q;

/**********************************************************
*                  STATIC VARIABLES
**********************************************************/
static QueueSetHandle_t incoming_events_q;
static const char       TAG[] = "STATE_CORE";

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
        BaseType_t    xStatus;

        xStatus = xQueueReceive(incoming_events_q, (void*)&rx, portMAX_DELAY);
        if (xStatus != pdTRUE) {
            ESP_LOGE(TAG, "Failed to rx... can't recover..");
            ASSERT(0);
        }

        ESP_LOGI(TAG, "RXed an event! %d", rx);

        xStatus = xQueueSendToBack(events_net_q, (void*)&rx, RTOS_DONT_WAIT);
        if (xStatus != pdTRUE) {
            ESP_LOGE(TAG, "Failed to read queue! (net)");
            ASSERT(0);
        }
    }
}

static void state_core_init_freertos_objects() {
    //Reads and Pushes events from state-machines
    incoming_events_q = xQueueCreate(EVENT_QUEUE_MAX_DEPTH, sizeof(state_event_t)); // state-machines -> state-core
    events_net_q      = xQueueCreate(EVENT_QUEUE_MAX_DEPTH, sizeof(state_event_t)); // state-core     -> net-sm

    // make sure nothing is NULL!
    ASSERT(incoming_events_q);
    ASSERT(events_net_q);
}

void state_post_event(state_event_t event) {
    BaseType_t xStatus = xQueueSendToBack(incoming_events_q, (void*)&event, RTOS_DONT_WAIT);
    if (xStatus != pdTRUE) {
        ESP_LOGE(TAG, "Failed to enqueue to dbus!");
        ASSERT(0);
    }
}

static void state_machine(void* arg) {
    if (!arg) {
        ESP_LOGE(TAG, "ARG = NULL!");
        ASSERT(0);
    }

    state_init_s* state_init_ptr = (state_init_s*)(arg);
    state_t       state          = state_init_ptr->starting_state;
    state_event_t new_event;

    for (;;) {
        // Execute current function
        state_init_ptr->translator(state)();

        // Check for new events
        new_event = state_init_ptr->get_event();
        if (new_event == INVALID_EVENT) {
            // No new event, sleep
            vTaskDelay(state_init_ptr->tick_period_ms / portTICK_PERIOD_MS);
            continue;
        }
        // Recieved an event, see if we need to change state
        state_init_ptr->next_state(&state, new_event);

        // Reset new_event
        new_event = INVALID_EVENT;
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

void start_new_state_machine(state_init_s* state_ptr) {
    if (!state_ptr) {
        ESP_LOGE(TAG, "ARG==NULL!");
        ASSERT(0);
    }
    
    ESP_LOGI(TAG, "Starting new state %s", state_ptr->state_name_string);
    BaseType_t rc = xTaskCreate(state_machine,
                                state_ptr->state_name_string,
                                4096,
                                (void*) state_ptr,
                                4,
                                NULL);

    if (rc != pdPASS) {
        ASSERT(0);
    }
}
