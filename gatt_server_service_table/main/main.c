#include "assert.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "nvs_flash.h"
#include <string.h>
#include <sys/param.h>

#include "state_core.h"
#include "net_state.h"
#include "wifi_state.h"
#include "ble_core.h"
#include "file_core.h"

int app_main(){
    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    //state_core_spawner();
    //net_state_spawner(); 

    ble_init();
    file_core_spawner();
/* 
    commandQ_file_t commandQ_cmd;
    for(int i = 0; i < PROVISION_CHUNK_SIZE; i++){
      commandQ_cmd.provision_chunk[i] = i;
    } 

  
    equeue_write(&commandQ_cmd);

    memset(&commandQ_cmd, 0, PROVISION_CHUNK_SIZE);

    vTaskDelay( 2000 / portTICK_PERIOD_MS);

    fetch_nvs(&commandQ_cmd);

    for(int i = 0; i < PROVISION_CHUNK_SIZE; i++){
      ESP_LOGI(TAG, "%d", commandQ_cmd.provision_chunk[i]);
    } 

    //state_post_event(wifi_connect);
    
    
    //state_post_event(wifi_disconnect);
*/
    return(0);
}
