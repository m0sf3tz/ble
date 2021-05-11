#include "argtable3/argtable3.h"
#include "driver/i2c.h"
#include "esp_console.h"
#include "esp_log.h"
#include "math.h"
#include "string.h"
#include <stdio.h>

#include "thermal.h"
#include "global_defines.h"
/**********************************************************
*                                  THERM STATIC VARIABLES *
**********************************************************/
static gpio_num_t        i2c_gpio_sda  = 18; //white
static gpio_num_t        i2c_gpio_scl  = 19;
static uint32_t          i2c_frequency = 100000;
static i2c_port_t        i2c_port      = I2C_NUM_0;
static const char        TAG[]         = "THERM";


static esp_err_t i2c_master_driver_initialize(void) {
    i2c_config_t conf = {
        .mode             = I2C_MODE_MASTER,
        .sda_io_num       = i2c_gpio_sda,
        .sda_pullup_en    = GPIO_PULLUP_ENABLE,
        .scl_io_num       = i2c_gpio_scl,
        .scl_pullup_en    = GPIO_PULLUP_ENABLE,
        .master.clk_speed = i2c_frequency
    };
    return i2c_param_config(i2c_port, &conf);
}

void instruction_write(uint8_t data) {
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (0x7C), ACK_CHECK_EN);
    i2c_master_write_byte(cmd, (0x00), ACK_CHECK_EN);
    i2c_master_write_byte(cmd, data, ACK_CHECK_EN);
    i2c_master_stop(cmd);
    i2c_master_cmd_begin(i2c_port, cmd, 50 / portTICK_RATE_MS);
    i2c_cmd_link_delete(cmd);
    vTaskDelay(10 / portTICK_PERIOD_MS);
}


static esp_err_t i2c_master_read_slave(uint8_t *data, size_t size, int reg)
{
    if (size == 0) {
        return ESP_OK;
    }

    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, SLAVE_ADDR << 1 | WRITE_BIT, ACK_CHECK_EN);
    i2c_master_write_byte(cmd, reg, ACK_CHECK_EN);
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, SLAVE_ADDR << 1 | READ_BIT, ACK_CHECK_EN);
    if (size > 1) {
        i2c_master_read(cmd, data, size - 1, ACK_VAL);
    }
    i2c_master_read_byte(cmd, data + size - 1, NACK_VAL);
    i2c_master_stop(cmd);
    esp_err_t ret = i2c_master_cmd_begin(i2c_port, cmd, 50 / portTICK_RATE_MS);
    i2c_cmd_link_delete(cmd);


/*
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (SLAVE_ADDR << 1) | READ_BIT, ACK_CHECK_EN);
    i2c_master_write_byte(cmd, reg, ACK_CHECK_EN);
    if (size > 1) {
        i2c_master_read(cmd, data_rd, size - 1, ACK_VAL);
    }
    i2c_master_read_byte(cmd, data_rd + size - 1, NACK_VAL);
    i2c_master_stop(cmd);
    esp_err_t ret = i2c_master_cmd_begin(i2c_port, cmd, 1000 / portTICK_RATE_MS);
    i2c_cmd_link_delete(cmd);
*/
    return ret;
}

int read_pixel(uint8_t pixel){
  if (0x80 > pixel){
    ESP_LOGE(TAG, "PIXEL ERROR RANGE!");
    ASSERT(0);
  }
  uint8_t temperature[2]; 
  i2c_master_read_slave(temperature, 2, pixel);

  uint16_t full = temperature[0] | (temperature[1] << 8);
 
  // check if negative
  if (temperature[1] >> 3){
    full = ~full + 1;
    full = full * -1;
  } 
  return full/4;
}

static void init_therm() {
    i2c_driver_install(i2c_port, I2C_MODE_MASTER, I2C_MASTER_RX_BUF_DISABLE, I2C_MASTER_TX_BUF_DISABLE, 0);
    i2c_master_driver_initialize();
}


void thermal_init(){
  ESP_LOGI(TAG, "Starting thermal init!");
  init_therm();
}

void get_thermal_image(uint8_t * img){
  ESP_LOGI(TAG, "getting thermal image!");

  if(img == NULL){
    ESP_LOGE(TAG, "Error, NULL arg!");
    ASSERT(0);
  }
  
  uint8_t * iter = img;
  for(int i = 0; i < 8; i++){
      for (int j = 0; j < 8; j++){
       *iter = read_pixel(0x80 + i*16 + j*2);
       printf(" %d ", *iter);
       iter++;
       if (j == 7){
        printf("\n");
       }
      }
    }
}

void thermal_test(){
  ESP_LOGI(TAG, "Starting test!");
  init_therm();

  for(;;){
    vTaskDelay(100);
    printf("\n");
    for(int i = 0; i < 8; i++){
      for (int j = 0; j < 8; j++){
       printf(" %d ", read_pixel(0x80 + i*16 + j*2));
       if (j == 7){
        printf("\n");
       }
      }
    }
  }
}

