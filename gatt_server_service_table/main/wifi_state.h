#pragma once
#include "state_core.h"

/**********************************************************
*                   GLOBAL FUNCTIONS
**********************************************************/

/**********************************************************
*                      GLOBALS    
*********************************************************/

/*********************************************************
*                     TYPEDEFS
**********************************************************/

/**********************************************************
*                      DEFINES
**********************************************************/
#define WIFI_STATE_BASE_OFFSET (100) // all events are global
                                     // we need to set this 
                                     // as not too conlfict
                                     // in namespace
/*********************************************************
*                      ENUMS
**********************************************************/
typedef enum{
  wifi_disconnect = WIFI_STATE_BASE_OFFSET,
  wifi_connect,

  wifi_final_state //LEAVE AS LAST!
}wifi_event_e;

