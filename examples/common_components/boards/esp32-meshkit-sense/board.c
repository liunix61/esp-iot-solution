// Copyright 2020-2021 Espressif Systems (Shanghai) PTE LTD
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at

//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <stdio.h>
#include "board.h"
#include "esp_log.h"

static const char *TAG = "Board";

static bool s_board_is_init = false;
static bool s_board_gpio_isinit = false;

#define BOARD_CHECK(a, str, ret) if(!(a)) { \
        ESP_LOGE(TAG,"%s:%d (%s):%s", __FILE__, __LINE__, __FUNCTION__, str); \
        return (ret); \
    }

/****Private board level API ****/
static i2c_bus_handle_t s_i2c0_bus_handle = NULL;
static i2c_bus_handle_t s_spi2_bus_handle = NULL;

static esp_err_t board_i2c_bus_init(void)
{
#if(CONFIG_BOARD_I2C0_INIT)
    i2c_config_t board_i2c_conf = {
        .mode = BOARD_I2C0_MODE,
        .sda_io_num = BOARD_IO_I2C0_SDA,
        .sda_pullup_en = BOARD_I2C0_SDA_PULLUP_EN,
        .scl_io_num = BOARD_IO_I2C0_SCL,
        .scl_pullup_en = BOARD_I2C0_SCL_PULLUP_EN,
        .master.clk_speed = BOARD_I2C0_SPEED,
    };
    i2c_bus_handle_t handle = i2c_bus_create(I2C_NUM_0, &board_i2c_conf);
    BOARD_CHECK(handle != NULL, "i2c_bus creat failed", ESP_FAIL);
    s_i2c0_bus_handle = handle;
#else
    s_i2c0_bus_handle = NULL;
#endif
    return ESP_OK;
}

static esp_err_t board_i2c_bus_deinit(void)
{
    if (s_i2c0_bus_handle != NULL) {
        i2c_bus_delete(&s_i2c0_bus_handle);
        BOARD_CHECK(s_i2c0_bus_handle == NULL, "i2c_bus delete failed", ESP_FAIL);
    }
    return ESP_OK;
}

static esp_err_t board_spi_bus_init(void)
{
#if(CONFIG_BOARD_SPI2_INIT)
    spi_config_t bus_conf = {
        .miso_io_num = BOARD_IO_SPI2_MISO,
        .mosi_io_num = BOARD_IO_SPI2_MOSI,
        .sclk_io_num = BOARD_IO_SPI2_SCK,
    };
    s_spi2_bus_handle = spi_bus_create(SPI2_HOST, &bus_conf);
    BOARD_CHECK(s_spi2_bus_handle != NULL, "spi_bus2 creat failed", ESP_FAIL);
#endif
    return ESP_OK;
}

static esp_err_t board_spi_bus_deinit(void)
{
    if (s_spi2_bus_handle != NULL) {
        spi_bus_delete(&s_spi2_bus_handle);
        BOARD_CHECK(s_spi2_bus_handle == NULL, "i2c_bus delete failed", ESP_FAIL);
    }
    return ESP_OK;
}

static esp_err_t board_gpio_init(void)
{
    if (s_board_gpio_isinit) {
        return ESP_OK;
    }
    gpio_config_t io_conf;
    //disable interrupt
    io_conf.intr_type = GPIO_INTR_DISABLE;
    //set as output mode
    io_conf.mode = GPIO_MODE_OUTPUT;
    //bit mask of the pins that you want to set
    io_conf.pin_bit_mask = BOARD_IO_PIN_SEL_OUTPUT;
    //disable pull-down mode
    io_conf.pull_down_en = 0;
    //disable pull-up mode
    io_conf.pull_up_en = 0;
    //configure GPIO with the given settings
    esp_err_t ret = gpio_config(&io_conf);
    if (ret == ESP_OK) {
        s_board_gpio_isinit = true;
    }
    return ret;
}

static esp_err_t board_gpio_deinit(void)
{
    if (!s_board_gpio_isinit) {
        return ESP_OK;
    }
    s_board_gpio_isinit = false;
    return ESP_OK;
}

/****General board level API ****/
esp_err_t iot_board_init(void)
{
    if(s_board_is_init) {
        return ESP_OK;
    }
    esp_err_t ret = board_gpio_init();
    BOARD_CHECK(ret == ESP_OK, "gpio init failed", ret);

    ret = board_i2c_bus_init();
    BOARD_CHECK(ret == ESP_OK, "i2c init failed", ret);

    ret = board_spi_bus_init();
    BOARD_CHECK(ret == ESP_OK, "spi init failed", ret);

#ifdef CONFIG_BOARD_POWER_SENSOR
    iot_board_sensor_set_power(true);
#else
    iot_board_sensor_set_power(false);
#endif

#ifdef CONFIG_BOARD_POWER_SCREEN
    iot_board_screen_set_power(true);
#else
    iot_board_screen_set_power(false);
#endif

    s_board_is_init = true;
    ESP_LOGI(TAG,"Board Info: %s", iot_board_get_info());
    ESP_LOGI(TAG,"Init Done ...");
    return ESP_OK;
}

esp_err_t iot_board_deinit(void)
{
    if(!s_board_is_init) {
        return ESP_OK;
    }
    esp_err_t ret = board_gpio_deinit();
    BOARD_CHECK(ret == ESP_OK, "gpio de-init failed", ret);

    ret = board_i2c_bus_deinit();
    BOARD_CHECK(ret == ESP_OK, "i2c de-init failed", ret);

    ret = board_spi_bus_deinit();
    BOARD_CHECK(ret == ESP_OK, "spi de-init failed", ret);

#ifdef CONFIG_BOARD_POWER_SENSOR
    iot_board_sensor_set_power(false);
#endif

#ifdef CONFIG_BOARD_POWER_SCREEN
    iot_board_screen_set_power(false);
#endif
    s_board_is_init = false;
    ESP_LOGI(TAG," Deinit Done ...");
    return ESP_OK;
}

bool iot_board_is_init(void)
{
    return s_board_is_init;
}

board_res_handle_t iot_board_get_handle(board_res_id_t id)
{
    board_res_handle_t handle;
    switch (id)
    {
    case BOARD_I2C0_ID:
        handle = (board_res_handle_t)s_i2c0_bus_handle;
        break;
    default:
        handle = NULL;
        break;
    }
    return handle;
}

char* iot_board_get_info()
{
    static char* info = BOARD_NAME;
    return info;
}

/****Extended board level API ****/
esp_err_t iot_board_sensor_set_power(bool on_off)
{
    if (!s_board_gpio_isinit) {
        return ESP_FAIL;
    }
    return gpio_set_level(BOARD_IO_POWER_ON_SENSOR_N, !on_off);
}

bool iot_board_sensor_get_power(void)
{
    if (!s_board_gpio_isinit) {
        return 0;
    }
    return !gpio_get_level(BOARD_IO_POWER_ON_SENSOR_N);
}

esp_err_t iot_board_screen_set_power(bool on_off)
{
    if (!s_board_gpio_isinit) {
        return ESP_FAIL;
    }
    return gpio_set_level(BOARD_IO_POWER_ON_SCREEN_N, !on_off);
}

bool iot_board_screen_get_power(void)
{
    if (!s_board_gpio_isinit) {
        return 0;
    }
    return gpio_get_level(BOARD_IO_POWER_ON_SCREEN_N);
}
