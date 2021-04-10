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
*                                        STATIC VARIABLES *
**********************************************************/
static const char TAG[] = "FILE_CORE";

static SemaphoreHandle_t nvs_sem;
static QueueHandle_t     file_command_q;

static const unsigned short crc16tab[256] = {
    0x0000, 0x1021, 0x2042, 0x3063, 0x4084, 0x50a5, 0x60c6, 0x70e7,
    0x8108, 0x9129, 0xa14a, 0xb16b, 0xc18c, 0xd1ad, 0xe1ce, 0xf1ef,
    0x1231, 0x0210, 0x3273, 0x2252, 0x52b5, 0x4294, 0x72f7, 0x62d6,
    0x9339, 0x8318, 0xb37b, 0xa35a, 0xd3bd, 0xc39c, 0xf3ff, 0xe3de,
    0x2462, 0x3443, 0x0420, 0x1401, 0x64e6, 0x74c7, 0x44a4, 0x5485,
    0xa56a, 0xb54b, 0x8528, 0x9509, 0xe5ee, 0xf5cf, 0xc5ac, 0xd58d,
    0x3653, 0x2672, 0x1611, 0x0630, 0x76d7, 0x66f6, 0x5695, 0x46b4,
    0xb75b, 0xa77a, 0x9719, 0x8738, 0xf7df, 0xe7fe, 0xd79d, 0xc7bc,
    0x48c4, 0x58e5, 0x6886, 0x78a7, 0x0840, 0x1861, 0x2802, 0x3823,
    0xc9cc, 0xd9ed, 0xe98e, 0xf9af, 0x8948, 0x9969, 0xa90a, 0xb92b,
    0x5af5, 0x4ad4, 0x7ab7, 0x6a96, 0x1a71, 0x0a50, 0x3a33, 0x2a12,
    0xdbfd, 0xcbdc, 0xfbbf, 0xeb9e, 0x9b79, 0x8b58, 0xbb3b, 0xab1a,
    0x6ca6, 0x7c87, 0x4ce4, 0x5cc5, 0x2c22, 0x3c03, 0x0c60, 0x1c41,
    0xedae, 0xfd8f, 0xcdec, 0xddcd, 0xad2a, 0xbd0b, 0x8d68, 0x9d49,
    0x7e97, 0x6eb6, 0x5ed5, 0x4ef4, 0x3e13, 0x2e32, 0x1e51, 0x0e70,
    0xff9f, 0xefbe, 0xdfdd, 0xcffc, 0xbf1b, 0xaf3a, 0x9f59, 0x8f78,
    0x9188, 0x81a9, 0xb1ca, 0xa1eb, 0xd10c, 0xc12d, 0xf14e, 0xe16f,
    0x1080, 0x00a1, 0x30c2, 0x20e3, 0x5004, 0x4025, 0x7046, 0x6067,
    0x83b9, 0x9398, 0xa3fb, 0xb3da, 0xc33d, 0xd31c, 0xe37f, 0xf35e,
    0x02b1, 0x1290, 0x22f3, 0x32d2, 0x4235, 0x5214, 0x6277, 0x7256,
    0xb5ea, 0xa5cb, 0x95a8, 0x8589, 0xf56e, 0xe54f, 0xd52c, 0xc50d,
    0x34e2, 0x24c3, 0x14a0, 0x0481, 0x7466, 0x6447, 0x5424, 0x4405,
    0xa7db, 0xb7fa, 0x8799, 0x97b8, 0xe75f, 0xf77e, 0xc71d, 0xd73c,
    0x26d3, 0x36f2, 0x0691, 0x16b0, 0x6657, 0x7676, 0x4615, 0x5634,
    0xd94c, 0xc96d, 0xf90e, 0xe92f, 0x99c8, 0x89e9, 0xb98a, 0xa9ab,
    0x5844, 0x4865, 0x7806, 0x6827, 0x18c0, 0x08e1, 0x3882, 0x28a3,
    0xcb7d, 0xdb5c, 0xeb3f, 0xfb1e, 0x8bf9, 0x9bd8, 0xabbb, 0xbb9a,
    0x4a75, 0x5a54, 0x6a37, 0x7a16, 0x0af1, 0x1ad0, 0x2ab3, 0x3a92,
    0xfd2e, 0xed0f, 0xdd6c, 0xcd4d, 0xbdaa, 0xad8b, 0x9de8, 0x8dc9,
    0x7c26, 0x6c07, 0x5c64, 0x4c45, 0x3ca2, 0x2c83, 0x1ce0, 0x0cc1,
    0xef1f, 0xff3e, 0xcf5d, 0xdf7c, 0xaf9b, 0xbfba, 0x8fd9, 0x9ff8,
    0x6e17, 0x7e36, 0x4e55, 0x5e74, 0x2e93, 0x3eb2, 0x0ed1, 0x1ef0
};

/**********************************************************
*                                        GLOBAL VARIABLES *
**********************************************************/

/**********************************************************
*                                        STATIC FUNCTIONS *
**********************************************************/
static uint16_t crc16(uint8_t* buf, size_t len) {
    ESP_LOGI(TAG, "Calculating CRC for len %d", len);

    int      counter;
    uint16_t crc = 0xBEEF;

    if (!buf) {
        ESP_LOGE(TAG, "CRC buf null!");
        ASSERT(0);
    }

    for (counter = 0; counter < len; counter++) {
        crc = (crc << 8) ^ crc16tab[((crc >> 8) ^ buf[counter]) & 0x00FF];
    }
    return crc;
}

static void file_core_init_freertos_objects() {
    nvs_sem        = xSemaphoreCreateMutex();
    file_command_q = xQueueCreate(1, sizeof(commandQ_file_t));

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

static void update_nvs(commandQ_file_t* commandQ_cmd) {
    uint16_t crc16_calc     = 0;
    uint16_t crc16_expected = 0;

    if (!commandQ_cmd) {
        ESP_LOGE(TAG, "PARAM NULL!");
        ASSERT(0);
    }

    crc16_expected = commandQ_cmd->crc_16;
    crc16_calc     = crc16(commandQ_cmd->provision_chunk, PROVISION_CHUNK_SIZE);
    ESP_LOGI(TAG, "CRC16(calculated) == %hu, CRC16(expected) == %hu", crc16_calc, crc16_expected);

    if (crc16_calc != crc16_expected) {
        ESP_LOGE(TAG, "Error! Won't commit! CRC ERROR!");
        return;
    } else {
        ESP_LOGI(TAG, "CRC GOOD!");
    }

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

        ESP_LOGI(TAG, "received a command for file_core!");

        // we only expect to get one type of command - and that's a command to commit to memory
        // we won't check for an error, but when the phone reads back to verify the write it will
        // see the failure then
        update_nvs(&commandQ_cmd);
    }
}

/**********************************************************
*                                        GLOBAL FUNCTIONS *
**********************************************************/
// If device is provisioned, breaks down the blob and
// returns a particular value based on a key IE,
// Key -> IP_KEY, fetches the IP value from the blob
int get_provision_item(char* dest, uint8_t key) {
    int     rc;
    uint8_t buf[PROVISION_CHUNK_SIZE];

    if (!dest) {
        ESP_LOGE(TAG, "PARAM NULL!");
        ASSERT(0);
    }

    if (key > MAX_KEY) {
        ESP_LOGE(TAG, "KEY WRONG!");
        ASSERT(0);
    }

    memset(buf, 0, PROVISION_CHUNK_SIZE);

    ESP_LOGI(TAG, "fetching from memory!");
    rc = file_core_get(NVS_CHUNK, buf);
    if (rc != ITEM_GOOD) {
        return rc;
    }

    switch (key) {
    case (IP_KEY):
        memcpy(dest, buf, IP_LEN);
        return ITEM_GOOD;
    case (SSID_KEY):
        memcpy(dest, buf + SSID_OFFSET, SSID_LEN);
        return ITEM_GOOD;
    case (PW_KEY):
        memcpy(dest, buf + PW_OFFSET, PW_LEN);
        return ITEM_GOOD;
    default:
        ESP_LOGE(TAG, "Key not handled...");
        ASSERT(0);
    }
}

BaseType_t equeue_write(commandQ_file_t* commandQ_cmd) {
    if (!commandQ_cmd) {
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
