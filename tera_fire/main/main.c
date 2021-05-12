#include "assert.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "nvs_flash.h"
#include <string.h>
#include <sys/param.h>

#include "ble_core.h"
#include "file_core.h"
#include "http_core.h"
#include "net_state.h"
#include "packet_core.h"
#include "state_core.h"
#include "wifi_core.h"
#include "wifi_state.h"
#include "thermal.h"


// What we will store sensor readings into, also API key
// Will post this to backend
char buff[API_LEN + SENSOR_READING_LEN];

int app_main() {
    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    thermal_init();

    init_wifi();
    ble_init();
    file_core_spawner();

    state_core_spawner();
    net_state_spawner();
    wifi_state_spawner();

    while (true) {
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        if (get_wifi_state()){
          create_packet(buff);
          http_post();
        }
    }
   
    return (0);
}
