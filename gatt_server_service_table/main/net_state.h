#pragma once
#include "state_core.h"

/**********************************************************
*                   GLOBAL FUNCTIONS
**********************************************************/
void state_post_event(state_event_t event);
void net_state_spawner();

/**********************************************************
*                      GLOBALS    
*********************************************************/
extern QueueHandle_t outgoing_events_net_q;

/*********************************************************
*                     TYPEDEFS
**********************************************************/
typedef void (*func_ptr)(void);

/*********************************************************
*                      ENUMS
**********************************************************/
typedef enum{
  net_waiting_wifi = 0,
  net_waiting_prov,
  net_running,

  net_state_len //LEAVE AS LAST!
}net_state_e;

/**********************************************************
*                      DEFINES
**********************************************************/
#define MAX_QUEUE_DEPTH (16)
#define INVALID_EVENT (0xFFFFFFFF)
