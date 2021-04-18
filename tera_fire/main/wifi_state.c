#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "esp_wifi_default.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"
#include "lwip/err.h"
#include "lwip/sys.h"
#include "sdkconfig.h"
#include <string.h>

#include "file_core.h"
#include "global_defines.h"
#include "state_core.h"
#include "wifi_core.h"
#include "wifi_state.h"

/*********************************************************
*                                       STATIC VARIABLES *
*********************************************************/
static const char        TAG[] = "WIFI_CORE";
static char              ssid_name[SSID_LEN];
static char              ssid_pw[PW_LEN];
QueueSetHandle_t         events_wifi_q;
static SemaphoreHandle_t wifi_state_mutex;

/**********************************************************
*                                         STATE FUNCTIONS *
**********************************************************/
static void wifi_wait_for_provision() {
    ESP_LOGI(TAG, "Entering wifi_wait_for_provision state");

    int err = get_provision_item(ssid_name, SSID_KEY);
    if (err != ITEM_GOOD) {
        ESP_LOGW(TAG, "ssid name not set in nvs, can't connect");
        return;
    }

    err = get_provision_item(ssid_pw, PW_KEY);
    if (err != ITEM_GOOD) {
        ESP_LOGW(TAG, "ssid pw not set in nvs, can't connect");
        return;
    }

    state_post_event(wifi_new_provision_event);
}

static void wifi_start_connect() {
    ESP_LOGI(TAG, "Entering wifi_start_connect state");
    wifi_core_connect();
}

static void wifi_stop_connect() {
    ESP_LOGI(TAG, "Entering wifi_stop state");
    wifi_core_stop();

    state_post_event(wifi_reset_event);
}

// Returns the next state
static void next_state_func(state_t* curr_state, state_event_t event) {
    if (!curr_state) {
        ESP_LOGE(TAG, "ARG= NULL!");
        ASSERT(0);
    }

    // Always reset
    if (event == wifi_reset_event) {
        *curr_state = wifi_waiting_provision_state;
        return;
    }

    if (*curr_state == wifi_waiting_provision_state) {
        ESP_LOGE(TAG, "A, %d", event);
        if (event == wifi_new_provision_event) {
            ESP_LOGE(TAG, "B");
            *curr_state = wifi_connecting_state;
            return;
        }
    }

    // go back and start again
    if (*curr_state == wifi_connecting_state) {
        if (event == wifi_new_provision_event) {
            *curr_state = wifi_stop_state;
            return;
        }
    }

    // Stay in the same state
}

char* event_print_func(state_event_t event) {
    // event not targeted at this state machine
    return NULL;
}

// Returns the state function, given a state
static state_array_s get_state_func(state_t state) {
    static state_array_s func_table[wifi_state_len] = {
        // clang-format off
        { wifi_wait_for_provision, 5000 / portTICK_PERIOD_MS },
        { wifi_start_connect     , portMAX_DELAY             },
        { wifi_stop_connect      , portMAX_DELAY             }
        // clang format on    
  };

    if (state >= wifi_state_len) {
        ESP_LOGE(TAG, "Current state out of bounds!");
        ASSERT(0);
    }

    return func_table[state];
}

static void send_event_func(state_event_t event) {
    BaseType_t xStatus = xQueueSendToBack(events_wifi_q, &event, WIFI_STATE_QUEUE_TO);
    if (xStatus != pdTRUE) {
        ESP_LOGE(TAG, "Failed to send on event queue (wifi)");
        ASSERT(0);
    }
}

static state_event_t get_event_func(uint32_t timeout) {
    state_event_t new_event = INVALID_EVENT;
    xQueueReceive(events_wifi_q, &new_event, timeout);
    return new_event;
}

static void wifi_state_init_freertos_objects() {
    wifi_state_mutex = xSemaphoreCreateMutex();
    events_wifi_q    = xQueueCreate(EVENT_QUEUE_MAX_DEPTH, sizeof(state_event_t)); // state-core     -> net-sm

    // make sure we init all the rtos objects
    ASSERT(wifi_state_mutex);
    ASSERT(events_wifi_q);
}

static bool event_filter_func(state_event_t event) {
    return true;
}

state_init_s* get_wifi_state_handle() {
    static state_init_s wifi_state = {
        .next_state        = next_state_func,
        .send_event        = send_event_func,
        .get_event         = get_event_func,
        .translator        = get_state_func,
        .event_print       = event_print_func,
        .starting_state    = wifi_waiting_provision_state,
        .state_name_string = "wifi_state",
        .filter_event      = event_filter_func,
    };
    return &(wifi_state);
}

void wifi_state_spawner() {
    wifi_state_init_freertos_objects();

    // State the state machine
    start_new_state_machine(get_wifi_state_handle());
}
