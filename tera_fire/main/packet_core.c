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
#include "packet_core.h"
#include "thermal.h" 

/*********************************************************
*                                       STATIC VARIABLES *
*********************************************************/
static const char TAG[] = "PACKET_CORE";

/**********************************************************
*                                          IMPLEMENTATION *
**********************************************************/
bool create_packet(char* buf) {
    if (!buf) {
        ESP_LOGI(TAG, "PARAM NULL!");
        ASSERT(0);
    }

    memset(buf, 0, API_LEN + SENSOR_READING_LEN);

    // Get the API key
    int err = get_provision_item(buf, API_KEY);
    if (err != ITEM_GOOD) {
        ESP_LOGE(TAG, "API key not set in nvs, can't creat packet");
        return false;
    }

    get_thermal_image((uint8_t*)(buf + API_LEN));
    return true;
}
