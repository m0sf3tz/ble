#pragma once

#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

/*********************************************************
*                     TYPEDEFS
**********************************************************/
typedef uint32_t state_event_t;
typedef uint32_t state_t;

// Individual state functions in a state machine
typedef void (*func_ptr)(void);

// Init function, used to set up a state machine
struct {

    // This is the function that calculates the next state, based on input
    void (*next_state)(state_t*, state_event_t);

    // How often to check if it's time to enter the next state
    uint16_t tick_period_ms;

    // This function reads events to this state machine
    state_event_t (*get_event)(void);

    // Initial state of the state machine
    state_t starting_state;

    // Translates a state_e item to a state function
    func_ptr (*translator)(state_t);

    // Translates a event to a string (just for debug)
    char* (*event_print)(state_event_t);

    // For debug, name of the state
    char* state_name_string;
    
} typedef state_init_s;

/**********************************************************
*                   GLOBAL FUNCTIONS
**********************************************************/
void state_post_event(state_event_t event);
void state_core_spawner();
void start_new_state_machine(state_init_s* state_ptr);

/**********************************************************
*                      GLOBALS    
*********************************************************/
extern QueueHandle_t events_net_q;

/**********************************************************
*                      DEFINES
**********************************************************/
#define INVALID_EVENT   (0xFFFFFFFF)
#define EVENT_QUEUE_MAX_DEPTH (16)
