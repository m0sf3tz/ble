#include "argtable3/argtable3.h"
#include "driver/i2c.h"
#include "esp_console.h"
#include "esp_log.h"
#include "math.h"
#include "string.h"
#include <stdio.h>

#include "lcd.h"
#include "system_defines.h"

#define I2C_MASTER_TX_BUF_DISABLE 0                /*!< I2C master doesn't need buffer */
#define I2C_MASTER_RX_BUF_DISABLE 0                /*!< I2C master doesn't need buffer */
#define WRITE_BIT                 I2C_MASTER_WRITE /*!< I2C master write */
#define READ_BIT                  I2C_MASTER_READ  /*!< I2C master read */
#define ACK_CHECK_EN              0x1              /*!< I2C master will check ack from slave*/
#define ACK_CHECK_DIS             0x0              /*!< I2C master will not check ack from slave */
#define ACK_VAL                   0x0              /*!< I2C ack value */
#define NACK_VAL                  0x1              /*!< I2C nack value */

#define RESET_PIN_LCD     (21)
#define RESET_PIN_LCD_SEL (1ULL << RESET_PIN_LCD)

/**********************************************************
*               LCD CORE GLOBAL VARIABLES
**********************************************************/
QueueHandle_t lcdPrintQ;

/**********************************************************
*              LCD CORE STATIC VARIABLES
**********************************************************/
static gpio_num_t        i2c_gpio_sda  = 18;
static gpio_num_t        i2c_gpio_scl  = 19;
static uint32_t          i2c_frequency = 100000;
static i2c_port_t        i2c_port      = I2C_NUM_0;
static const char        TAG[]         = "LCD_CORE";
static SemaphoreHandle_t lcd_state_mutex;
static uint8_t           lcd_state;

static void init_gpio_lcd() {
    // ********************* SET UP RESET PIN **********************
    gpio_config_t io_conf;
    memset(&io_conf, 0, sizeof(gpio_config_t));
    //disable interrupt
    io_conf.intr_type = GPIO_PIN_INTR_DISABLE;
    //set as output mode
    io_conf.mode = GPIO_MODE_OUTPUT;
    //bit mask of the pins that you want to set, DIG21 (IO21)
    io_conf.pin_bit_mask = RESET_PIN_LCD_SEL;
    //disable pull-down mode
    io_conf.pull_down_en = 0;
    //disable pull-up mode
    io_conf.pull_up_en = 0;
    //configure GPIO with the given settings
    gpio_config(&io_conf);
    //pull device out of reset
    gpio_set_level(RESET_PIN_LCD, 1);
}

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

static struct {
    struct arg_int* port;
    struct arg_int* freq;
    struct arg_int* sda;
    struct arg_int* scl;
    struct arg_end* end;
} i2cconfig_args;

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

void data_write(uint8_t data) {
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (0x7C), ACK_CHECK_EN);
    i2c_master_write_byte(cmd, (0x40), ACK_CHECK_EN);
    i2c_master_write_byte(cmd, data, ACK_CHECK_EN);
    i2c_master_stop(cmd);
    i2c_master_cmd_begin(i2c_port, cmd, 50 / portTICK_RATE_MS);
    i2c_cmd_link_delete(cmd);
}

static void init_lcd() {
    i2c_driver_install(i2c_port, I2C_MODE_MASTER, I2C_MASTER_RX_BUF_DISABLE, I2C_MASTER_TX_BUF_DISABLE, 0);
    i2c_master_driver_initialize();

    gpio_set_level(RESET_PIN_LCD, 0);
    vTaskDelay(100 / portTICK_PERIOD_MS);
    gpio_set_level(RESET_PIN_LCD, 1);
    vTaskDelay(1000 / portTICK_PERIOD_MS);

    ESP_LOGI(TAG, "init LCD start");
    instruction_write(0x38);
    instruction_write(0x39);
    instruction_write(0x14);
    instruction_write(0x79);
    instruction_write(0x54);
    instruction_write(0x6F);
    vTaskDelay(250 / portTICK_PERIOD_MS);
    instruction_write(0x0C);
    instruction_write(0x01);
    instruction_write(0x06);
    instruction_write(0x02);
    ESP_LOGI(TAG, "Done init LCD");
}

void clear_screen() {
    instruction_write(0x02); //return home
    instruction_write(0x01); //clear display
}

void next_line() {
    instruction_write(0x80 | 0x40); // set dram address to 0x40
}

void thermal_test(){

}
