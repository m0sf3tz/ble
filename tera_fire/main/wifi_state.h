#pragma once
#include "state_core.h"

/**********************************************************
*                                        GLOBAL FUNCTIONS *
**********************************************************/
void wifi_state_spawner();

/*********************************************************
*                                                GLOBALS *
*********************************************************/

/*********************************************************
*                                               TYPEDEFS *
*********************************************************/

/*********************************************************
*                                                DEFINES *
*********************************************************/
#define WIFI_STATE_BASE_OFFSET (300) /* all events are global \
                                        we need to set this   \
                                        as not too conlfict   \
                                        in namespace          \
                                      */

#define WIFI_STATE_QUEUE_TO (2500 / portTICK_PERIOD_MS)
/*********************************************************
*                                                 ENUMS  *
*********************************************************/
typedef enum {
    wifi_new_provision_event = WIFI_STATE_BASE_OFFSET,
    wifi_reset_event,

    wifi_event_len //LEAVE AS LAST!
} wifi_event_e;

typedef enum {
    wifi_waiting_provision_state = 0,
    wifi_connecting_state,
    wifi_stop_state,

    wifi_state_len //LEAVE AS LAST!
} wifi_state_e;
