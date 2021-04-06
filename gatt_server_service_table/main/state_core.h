#pragma once
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "esp_system.h"

/*********************************************************
*                     TYPEDEFS
**********************************************************/
typedef uint32_t state_event_t;
typedef uint32_t state_t;

/**********************************************************
*                   GLOBAL FUNCTIONS
**********************************************************/
void state_post_event(state_event_t event);
void state_core_spawner();

/**********************************************************
*                      GLOBALS    
*********************************************************/
extern QueueHandle_t events_net_q;

/**********************************************************
*                      DEFINES
**********************************************************/
#define MAX_QUEUE_DEPTH (16)
#define INVALID_EVENT (0xFFFFFFFF)
