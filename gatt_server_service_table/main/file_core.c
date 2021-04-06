#include "esp_system.h"
#include "esp_vfs.h"
#include "esp_vfs_fat.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <errno.h>

#include "nvs.h"
#include "nvs_flash.h"

#include "file_core.h"
#include "global_defines.h"


/**********************************************************
*              MASTER CORE STATIC VARIABLES
**********************************************************/
static const char  TAG[30]   = "FILE_CORE";

// local mutexes
static SemaphoreHandle_t nvs_sem;
static QueueHandle_t     file_command_q;
/**********************************************************
*              FILE CORE GLOBAL VARIABLES
**********************************************************/

/**********************************************************
*                  FILE CORE FUNCTIONS
**********************************************************/
static void file_core_init_freertos_objects() {
    nvs_sem          = xSemaphoreCreateMutex();
    file_command_q   = xQueueCreate(1, sizeof(commandQ_file_t));
    
    // make sure we init all the rtos objects
    ASSERT(file_command_q);
    ASSERT(nvs_sem);
}

static int file_core_set(int item, void* data) {
    if (pdTRUE != xSemaphoreTake(nvs_sem, FILE_MAX_MUTEX_WAIT)) {
        ESP_LOGE(TAG, "FAILED TO TAKE NVS_MUTEX!");
        ASSERT(0);
    }

    if (!data) {
        ESP_LOGE(TAG, "DATA == NULL");
        ASSERT(0);
    }

    nvs_handle_t my_handle;
    esp_err_t    err = nvs_open("nvs", NVS_READWRITE, &my_handle);
    if (err != ESP_OK) {
        printf("Error (%s) opening NVS handle!\n", esp_err_to_name(err));
        xSemaphoreGive(nvs_sem);
        return ITEM_CANT_SET;
    } else {
        switch (item) {
        case (NVS_CHUNK):
            printf("Saving provision chunk to NVS ... \n");
            err = nvs_set_blob(my_handle, "provision_chunk", (const void*)(data), PROVISION_CHUNK_SIZE);
            break;
        default:
            ESP_LOGE(TAG, "Unknown item = %d \n", item);
            ASSERT(0);
        }

        // Commit written value.
        // After setting any values, nvs_commit() must be called to ensure changes are written
        // to flash storage. Implementations may write to storage at other times,
        // but this is not guaranteed.
        if (err == ESP_OK) {
            printf("Committing updates in NVS ... \n");
            err = nvs_commit(my_handle);
            if (err != ESP_OK) {
                printf("err == %s", esp_err_to_name(err));
            } else {
                printf("no issues commiting\n");
            }
        }
        // Close
        xSemaphoreGive(nvs_sem);
        nvs_close(my_handle);
        return ITEM_GOOD;
    }
}

static int file_core_get(int item, void* data) {
    int    status;
    size_t size_of_prov = 0;

    if (!data) {
        ESP_LOGE(TAG, "DATA == NULL");
        ASSERT(0);
    }

    if (pdTRUE != xSemaphoreTake(nvs_sem, FILE_MAX_MUTEX_WAIT)) {
        ESP_LOGE(TAG, "FAILED TO TAKE NVS_MUTEX!");
        ASSERT(0);
    }

    printf("\n");
    printf("Opening Non-Volatile Storage (NVS) handle... ");
    nvs_handle_t my_handle;
    esp_err_t    err = nvs_open("nvs", NVS_READWRITE, &my_handle);
    if (err != ESP_OK) {
        printf("Error (%s) opening NVS handle!\n", esp_err_to_name(err));
        status = ITEM_OTHER_PROBLEM;
        xSemaphoreGive(nvs_sem);
        return ITEM_OTHER_PROBLEM;
    } else {
        // Read
        switch (item) {
        case (NVS_CHUNK):
            printf("Reading size of provision_chunk in NVS ... \n");
            err = nvs_get_blob(my_handle, "provision_chunk", NULL, &size_of_prov);
            if (err != ESP_OK) {
                printf("Failed to get provision_chunk for device \n");
                break;
            }
            printf("Reading provision_chunk in NVS of len %d... \n", size_of_prov);
            err = nvs_get_blob(my_handle, "provision_chunk", (void*)(data), &size_of_prov);
            break;
        default:
            ESP_LOGE(TAG, "Unknown item = %d \n", item);
            ASSERT(0);
        }
        
        switch (err) {
        case ESP_OK:
            printf("Done\n");
            status = ITEM_GOOD;
            nvs_close(my_handle);
            xSemaphoreGive(nvs_sem);
            return status;
        case ESP_ERR_NVS_NOT_FOUND:
            printf("Item not found!\n");
            status = ITEM_NOT_INITILIZED;
            nvs_close(my_handle);
            xSemaphoreGive(nvs_sem);
            return status;
        default:
            printf("Error (%s) reading!\n", esp_err_to_name(err));
            ASSERT(0);
        }
    }
    printf("Should not get here!!");
    ASSERT(0);
    return 0;
}

int verify_nvs_required_items() {
/*
  uint64_t device_id;
    int      ret_deviceid = file_core_get(NVS_DEVICE_ID, &device_id);

    char ssid_name[MAX_SSID_LEN];
    int  ret_ssid_name = file_core_get(NVS_SSID_NAME, ssid_name);

    char ssid_pw[MAX_PW_LEN];
    int  ret_ssid_pw = file_core_get(NVS_SSID_PW, ssid_pw);

    char ip[MAX_IP_LEN];
    int  ret_ip = file_core_get(NVS_IP, ip);

    uint16_t port;
    int      ret_port = file_core_get(NVS_PORT, &port);

    char device_name[MAX_DEVICE_NAME];
    int  ret_device_name = file_core_get(NVS_DEVICE_NAME, device_name);

    //check if this is the first time we are runnig (the BRICKED value will not be set..)
    uint8_t bricked;
    int  ret_bricked = file_core_get(NVS_BRICKED, &bricked);
    
      ESP_LOGI(TAG, "Bricked = %hhu, err = %d",bricked, ret_bricked);
    if(ret_bricked ==  ITEM_NOT_INITILIZED) {
      ESP_LOGI(TAG, "First time running device, setting bricked code to 0");
      bricked = 0;
      ret_bricked = file_core_set(NVS_BRICKED, &bricked);
      if (ret_bricked != ESP_OK){
        ESP_LOGE(TAG, "Could not set Value of briked on the first run?!");
        ASSERT(0);
      }
    }

    if (ret_deviceid || ret_ssid_name || ret_ssid_pw || ret_ip || ret_port || ret_device_name) {
        return 0;
    } else {
        return 1;
    }
*/
  return 1;
}

void file_core_print_details() {
/*
    uint64_t device_id;
    int      ret_deviceid = file_core_get(NVS_DEVICE_ID, &device_id);

    char ssid_name[MAX_SSID_LEN];
    int  ret_ssid_name = file_core_get(NVS_SSID_NAME, ssid_name);

    char ssid_pw[MAX_PW_LEN];
    int  ret_ssid_pw = file_core_get(NVS_SSID_PW, ssid_pw);

    char ip[MAX_IP_LEN];
    int  ret_ip = file_core_get(NVS_IP, ip);

    uint16_t port;
    int      ret_port = file_core_get(NVS_PORT, &port);

    char device_name[MAX_DEVICE_NAME];
    int  ret_device_name = file_core_get(NVS_DEVICE_NAME, device_name);

    if (ret_deviceid == ITEM_GOOD) {
        ESP_LOGI(TAG, "device id %u", (uint32_t)device_id);
    } else {
        ESP_LOGE(TAG, "device id not set!");
    }

    if (ret_ssid_name == ITEM_GOOD) {
        ESP_LOGI(TAG, "ssid name == %s", ssid_name);
    } else {
        ESP_LOGE(TAG, "ssid name not set!");
    }

    if (ret_ssid_pw == ITEM_GOOD) {
        ESP_LOGI(TAG, "pw == %s", ssid_pw);
    } else {
        ESP_LOGE(TAG, "pw not set!");
    }

    if (ret_ip == ITEM_GOOD) {
        ESP_LOGI(TAG, "ip == %s", ip);
    } else {
        ESP_LOGE(TAG, "IP not set!");
    }

    if (ret_port == ITEM_GOOD) {
        ESP_LOGI(TAG, "port == %hu", port);
    } else {
        ESP_LOGE(TAG, "port not set!");
    }

    if (ret_device_name == ITEM_GOOD) {
        ESP_LOGI(TAG, "device_name == %s", device_name);
    } else {
        ESP_LOGE(TAG, "device name not set!");
    }
*/
}

void fetch_nvs(commandQ_file_t* commandQ_cmd){
    if (!commandQ_cmd){
      ESP_LOGE(TAG, "PARAM NULL!");
      ASSERT(0);
    }
  
    ESP_LOGI(TAG, "fetching from memory!");
    file_core_get(NVS_CHUNK, commandQ_cmd);
}

static void update_nvs(commandQ_file_t* commandQ_cmd){
    if (!commandQ_cmd){
      ESP_LOGE(TAG, "PARAM NULL!");
      ASSERT(0);
    }
  
    ESP_LOGI(TAG, "Commiting command to memory!");
    file_core_set(NVS_CHUNK, commandQ_cmd);
}

static void file_thread(void* ptr) {
    commandQ_file_t commandQ_cmd;

    ESP_LOGI(TAG, "Starting file core!!");  
    for (;;) {
        // Wait for command
        BaseType_t xStatus = xQueueReceive(file_command_q, &commandQ_cmd, portMAX_DELAY);
        if (xStatus == 0) {
            ESP_LOGE(TAG, "Failed to read Queue!");
            ASSERT(0);
        }
        
        ESP_LOGI(TAG, "recived a command for file_core!");  

        // we only expect to get one type of command - and that's a command to commit to memory
        // we won't check for an error, but when the phone reads back to verify the write it will
        // see the failure then
        update_nvs( &commandQ_cmd );
    }
}

BaseType_t equeue_write(commandQ_file_t* commandQ_cmd){
    if (!commandQ_cmd){
      ESP_LOGE(TAG, "PARAM NULL!");
      ASSERT(0);
    }
 
    return xQueueSendToBack(file_command_q, commandQ_cmd, RTOS_DONT_WAIT);
}

void file_core_spawner() {
    file_core_init_freertos_objects();

    BaseType_t rc;
    rc = xTaskCreate(file_thread,
                     "file_core",
                     4096,
                     NULL,
                     4,
                     NULL);

    if (rc != pdPASS) {
        assert(0);
    }
}
