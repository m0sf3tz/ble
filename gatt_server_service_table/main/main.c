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
#include "net_state.h"
#include "state_core.h"
#include "wifi_state.h"

int app_main() {
    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    state_core_spawner();
    net_state_spawner();

    ble_init();
    file_core_spawner();

    while(true){
    state_post_event(wifi_connect);
    
    vTaskDelay(10000 / portTICK_PERIOD_MS);
    
    state_post_event(wifi_disconnect);
    
    vTaskDelay(10000 / portTICK_PERIOD_MS);
    }

    return (0);
}
