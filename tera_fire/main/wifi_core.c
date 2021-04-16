#include "wifi_core.h"
#include "esp_event.h"
#include "esp_wifi.h"
#include "esp_wifi_default.h"
#include "sdkconfig.h"
#include <string.h>
#include "esp_log.h"
#include "esp_netif.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"
#include "lwip/err.h"
#include "lwip/sys.h"

#include "file_core.h"
#include "global_defines.h"

/**********************************************************
*                                        STATIC VARIABLES *
**********************************************************/

static const char* TAG = "WIFI_CORE";
static uint32_t           wifi_state;
static esp_ip4_addr_t     s_ip_addr;
static const char*        s_connection_name;
static esp_netif_t*       s_example_esp_netif = NULL;
static char              ssid_name[SSID_LEN];
static char              ssid_pw[PW_LEN];
static esp_eth_handle_t s_eth_handle = NULL;
static esp_eth_mac_t*   s_mac        = NULL;
static esp_eth_phy_t*   s_phy        = NULL;
static void*            s_eth_glue   = NULL;

/**********************************************************
*                                    FORWARD DECLERATIONS *
**********************************************************/
static void start(void);
static void stop(void);

/**********************************************************
*                                         IMLPEMENTATIONS *
**********************************************************/

static void on_got_ip(void* arg, esp_event_base_t event_base,
                      int32_t event_id, void* event_data) {
   ip_event_got_ip_t* event = (ip_event_got_ip_t*)event_data;
   memcpy(&s_ip_addr, &event->ip_info.ip, sizeof(s_ip_addr));
   ESP_LOGI(TAG, "IPv4 address: " IPSTR, IP2STR(&s_ip_addr)); 
}

static void on_wifi_disconnect(void* arg, esp_event_base_t event_base,
                               int32_t event_id, void* event_data) {
    ESP_LOGI(TAG, "Wi-Fi disconnected, trying to reconnect...");
    esp_err_t err = esp_wifi_connect();
    if (err == ESP_ERR_WIFI_NOT_STARTED) {
        return;
    }
    ESP_ERROR_CHECK(err);
}

void wifi_core_connect(void) {
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    esp_netif_config_t netif_config = ESP_NETIF_DEFAULT_WIFI_STA();
    esp_netif_t* netif = esp_netif_new(&netif_config);
    assert(netif);
    esp_netif_attach_wifi_station(netif);
    esp_wifi_set_default_wifi_sta_handlers();
    s_example_esp_netif = netif;
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED, &on_wifi_disconnect, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &on_got_ip, NULL));
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
    wifi_config_t wifi_config = {
        .sta = {
            .ssid     = "NA",
            .password = "NA",
        },
    };

    // We should not get an error on read, we already know the device is provisioned 
    int err = get_provision_item(ssid_name , SSID_KEY);
    if (err != ITEM_GOOD) {
        ESP_LOGE(TAG, "ssid name not set in nvs, can't connect");
        ASSERT(0);
    }

    err = get_provision_item(ssid_pw, PW_KEY);
    if (err != ITEM_GOOD) {
        ESP_LOGE(TAG, "ssid name not set in nvs, can't connect");
        ASSERT(0);  
    }

    memcpy(wifi_config.sta.ssid, ssid_name, strlen(ssid_name));
    memcpy(wifi_config.sta.password, ssid_pw, strlen(ssid_pw));

    ESP_LOGI(TAG, "Connecting to %s...", wifi_config.sta.ssid);
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());
    ESP_ERROR_CHECK(esp_wifi_connect());
    s_connection_name = CONFIG_EXAMPLE_WIFI_SSID;
}

void wifi_core_stop(void) {
    ESP_ERROR_CHECK(esp_event_handler_unregister(WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED, &on_wifi_disconnect));
    ESP_ERROR_CHECK(esp_event_handler_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, &on_got_ip));
    esp_err_t err = esp_wifi_stop();
    if (err == ESP_ERR_WIFI_NOT_INIT) {
        return;
    }
    ESP_ERROR_CHECK(err);
    ESP_ERROR_CHECK(esp_wifi_deinit());
    ESP_ERROR_CHECK(esp_wifi_clear_default_wifi_driver_and_handlers(s_example_esp_netif));
    esp_netif_destroy(s_example_esp_netif);
    s_example_esp_netif = NULL;
}


int test_wifi(){
  while (true){
  wifi_core_connect();

  vTaskDelay( 15000 /portTICK_PERIOD_MS);
  
  wifi_core_stop();
  vTaskDelay( 15000 /portTICK_PERIOD_MS);
  }
  
  return 0;
}


