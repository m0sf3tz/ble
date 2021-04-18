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

int app_main() {
    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    ble_init();
    file_core_spawner();

    state_core_spawner();
    net_state_spawner();
    wifi_state_spawner();

    vTaskDelay(30000 / portTICK_PERIOD_MS);
    ESP_LOGI("TAG", "posting!");
    http_test();

    char test[700];

    while (true) {
        vTaskDelay(10000 / portTICK_PERIOD_MS);
        create_packet(test);
        http_test();
        vTaskDelay(10000 / portTICK_PERIOD_MS);
    }
    return (0);
}
