#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"

/**********************************************************
*                      GLOBALS    
*********************************************************/

/**********************************************************
*                      DEFINES
**********************************************************/
#define FILE_MAX_MUTEX_WAIT (5000 / portTICK_PERIOD_MS)

/* define responses from file core */
#define FILE_RET_OK             (0)
#define FILE_RET_FAIL           (10)
#define FILE_MEM_EMPTY          (11)

/* NVS related stuff */
#define ITEM_GOOD           (0)
#define ITEM_NOT_INITILIZED (1)
#define ITEM_OTHER_PROBLEM  (2)
#define ITEM_CANT_GET       (3)
#define ITEM_CANT_SET       (4)

#define NVS_CHUNK           (0)

#define MAX_MTU_SIZE      (512)

/* The provision chunk will ALLWAYS
   be this size, variable items (PW, etc)
   will be NULL padded to this size. This
   makes packing unpacking simpler.
   KEEP in sync with below!! */
#define IP_LEN            (4)    //IPv4 only
#define SSID_LEN          (100)  //NULL terminated
#define PW_LEN            (100)  //NULL terminated 
#define API_LEN           (64)     
#define MAX_DEVICE_ID_LEN (4)

/* Keep this sync with the list above! */
/* define keys for file_core */
#define IP_KEY        (0)
#define SSID_KEY      (1)
#define PW_KEY        (2)
#define API_KEY       (3)
#define DEVICE_KEY    (4)

#define PROVISION_CHUNK_SIZE (IP_LEN + SSID_LEN + PW_LEN + API_LEN + MAX_DEVICE_ID_LEN)

/*********************************************************
*                     TYPEDEFS
**********************************************************/
typedef struct{
  uint8_t provision_chunk[PROVISION_CHUNK_SIZE];
} commandQ_file_t;

/**********************************************************
*                   GLOBAL FUNCTIONS
**********************************************************/
void fetch_nvs(commandQ_file_t* commandQ_cmd);
void file_core_spawner(void);
BaseType_t equeue_write(commandQ_file_t* commandQ_cmd);
