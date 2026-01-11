/*
 * @file ttp229.c
 * @brief 2025科中考核ttp229驱动相关文件
 * @author 何荣新
 * 
 * @details 本文件内包含了按键触摸存储记录、反应等功能
 */

#include <ttp229.h>
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "esp_log.h"
#include "led_strip.h"
#include "driver/ledc.h"
#include "esp_timer.h"
#include "esp_random.h" 
#include <math.h>
#include <string.h>
#include "global.h"

//按键状态
typedef struct {
    int key_num;
    bool is_pressed;            
} KeyState;
KeyState key_states[16] = {0};

//TTP229初始化,初始化似乎确实直接搬过来不用改
void ttp229_init() {
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << TTP229_SCL_PIN),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&io_conf);

    io_conf.pin_bit_mask = (1ULL << TTP229_SDO_PIN);
    io_conf.mode = GPIO_MODE_INPUT;
    gpio_config(&io_conf);

    gpio_set_level(TTP229_SCL_PIN, 1);
}

//TTP229读取函数
uint16_t ttp229_read_reversed() {
    uint16_t keys = 0;
    gpio_set_level(TTP229_SCL_PIN, 0);
    
    for (int i = 15; i >= 0; i--) {
        gpio_set_level(TTP229_SCL_PIN, 1);
        if (!gpio_get_level(TTP229_SDO_PIN)) {
            keys |= (1 << (15 - i));
        }
        gpio_set_level(TTP229_SCL_PIN, 0);
    }
    
    gpio_set_level(TTP229_SCL_PIN, 1);
    return keys;
}


//初始化按键状态
void init_key_states() {
    for (int i = 0; i < 16; i++) {
        key_states[i].key_num = i + 1;
        key_states[i].is_pressed = false;
    }
}
