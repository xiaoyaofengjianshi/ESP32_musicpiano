/*
 * @file ws2812.c
 * @brief 2025科中考核WS2812B相关文件
 * @author 何荣新
 * 
 * @details 本文件是WS2812B相关功能的实现
 */

#include "ws2812.h"
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
#include "oled.h"

//key到LED的映射
int key_to_led_index[17] = {
    -1,//0: 未使用
    8,//1: Re# 
    9,//2: Re 
    10,//3: Do# 
    11,//4: Do 
    -1,//5: 音量增加
    -1,//6: 亮度增加
    -1,//7: 音量减少
    -1,//8: 亮度减少
    3,//9: So# 
    2,//10: La 
    1,//11: La# 
    0,//12: Si 
    4,//13: So 
    5,//14: Fa# 
    6,//15: Fa 
    7//16: Mi 
};

 led_strip_handle_t led_strip;//LED灯条
//WS2812 LED灯条
bool key_pressed[LIGHT_STRENGTH] = {false};//记录每个LED是否被按键按下

float light_level = 0.1f;//亮度

//灯带初始化
void light_start_set() {
    led_strip_config_t strip_config = {
        .strip_gpio_num = LIGHT_GPIO,
        .max_leds = LIGHT_STRENGTH,
    };

    led_strip_rmt_config_t rmt_config = {
        .resolution_hz = 10 * 1000 * 1000,//时钟基准频率
    };

    ESP_ERROR_CHECK(led_strip_new_rmt_device(&strip_config, &rmt_config, &led_strip));
    led_strip_clear(led_strip);
    led_strip_refresh(led_strip);
    vTaskDelay(100 / portTICK_PERIOD_MS);
}



//灯组亮度设置
void light_level_set(int index, uint8_t r, uint8_t g, uint8_t b) {
    uint8_t r_light = (uint8_t)(r * light_level);
    uint8_t g_light = (uint8_t)(g * light_level);
    uint8_t b_light = (uint8_t)(b * light_level);
    led_strip_set_pixel(led_strip, index, r_light, g_light, b_light);
}

//HSV到RGB转换函数
uint32_t led_strip_hsv2rgb(uint32_t h, uint32_t s, uint32_t v) {//to和2音相近，懒得打_to_了
    h %= 360;
    float s_f = s / 100.0f;
    float v_f = v / 100.0f;
    
    float c = v_f * s_f;
    float x = c * (1 - fabsf(fmodf(h / 60.0f, 2) - 1));
    float m = v_f - c;
    
    float r, g, b;
    if (h < 60) {
        r = c; g = x; b = 0;
    } else if (h < 120) {
        r = x; g = c; b = 0;
    } else if (h < 180) {
        r = 0; g = c; b = x;
    } else if (h < 240) {
        r = 0; g = x; b = c;
    } else if (h < 300) {
        r = x; g = 0; b = c;
    } else {
        r = c; g = 0; b = x;
    }
    
    return ((uint32_t)((r + m) * 255) << 16) | 
           ((uint32_t)((g + m) * 255) << 8) | 
           (uint32_t)((b + m) * 255);
}

//灯组彩虹效果
void rainbow(int speed) {
    static uint16_t hue = 0;
    
    for (int i = 0; i < LIGHT_STRENGTH; i++) {
        if (key_pressed[i]) {
            // 如果该LED对应的按键被按下，设置为灭（黑色）
            led_strip_set_pixel(led_strip, i, 0, 0, 0);
        } else {
            // 否则显示彩虹效果
            uint16_t led_hue = (hue + (i * 360 / LIGHT_STRENGTH)) % 360;
            uint32_t rgb_color = led_strip_hsv2rgb(led_hue, 100, 100);
            
            uint8_t r = (uint8_t)(((rgb_color >> 16) & 0xFF) * light_level);
            uint8_t g = (uint8_t)(((rgb_color >> 8) & 0xFF) * light_level);
            uint8_t b = (uint8_t)((rgb_color & 0xFF) * light_level);
            
            led_strip_set_pixel(led_strip, i, r, g, b);
        }
    }

    led_strip_refresh(led_strip);
    hue = (hue + 1) % 360;
    vTaskDelay(speed / portTICK_PERIOD_MS);
}


//开机初始化
void startup_rainbow() {
    int64_t start_time = esp_timer_get_time();
    int64_t duration = 1000000;
    ssd1306_clear_screen(0x00);
    ssd1306_refresh_gram();
    display_fullscreen_simple(fullscreen_bitmap);
    while(1) {
        rainbow(3);
        if (esp_timer_get_time() - start_time >= duration) {
            break;
        }
    }
}


//水波
void moving_wave_effect(void) {
    static int wave_position = 0;
    static int wave_direction = 1;
    static uint32_t wave_color = 0x0000FF;//蓝色
    static int64_t last_wave_update = 0;
    static const int WAVE_UPDATE_INTERVAL = 50;//更新间隔(毫秒)
    
    int64_t current_time = esp_timer_get_time();
    
//检查是否需要更新波效果
    if (current_time - last_wave_update < WAVE_UPDATE_INTERVAL * 1000) {
        return;
    }
    last_wave_update = current_time;
    
//清除所有LED
    led_strip_clear(led_strip);
    
//更新波的位置
    wave_position += wave_direction;

//转向与改色
    if (wave_position >= LIGHT_STRENGTH - 2) {
        wave_direction = -1;//改为反向
        static uint16_t hue = 0;
        hue = (hue + 60) % 360;
        wave_color = led_strip_hsv2rgb(hue, 100, 100);
    } else if (wave_position <= 0) {
        wave_direction = 1;//改为正向
        static uint16_t hue = 0;
        hue = (hue + 60) % 360;
        wave_color = led_strip_hsv2rgb(hue, 100, 100);
    }
    
//设LED颜色
    int center = wave_position;
    
//确保灯号在有效范围内
    if (center >= 0 && center < LIGHT_STRENGTH) {
//中心LED
        uint8_t r = (uint8_t)(((wave_color >> 16) & 0xFF) * light_level);
        uint8_t g = (uint8_t)(((wave_color >> 8) & 0xFF) * light_level);
        uint8_t b = (uint8_t)((wave_color & 0xFF) * light_level);
        led_strip_set_pixel(led_strip, center, r, g, b);
    }
    
//左右各一个LED
    if (center - 1 >= 0 && center - 1 < LIGHT_STRENGTH) {
//左侧LED（亮度较低）
        uint8_t r = (uint8_t)(((wave_color >> 16) & 0xFF) * 0.7f * light_level);
        uint8_t g = (uint8_t)(((wave_color >> 8) & 0xFF) * 0.7f * light_level);
        uint8_t b = (uint8_t)((wave_color & 0xFF) * 0.7f * light_level);
        led_strip_set_pixel(led_strip, center - 1, r, g, b);
    }
    
    if (center + 1 >= 0 && center + 1 < LIGHT_STRENGTH) {
//右侧LED（亮度较低）
        uint8_t r = (uint8_t)(((wave_color >> 16) & 0xFF) * 0.7f * light_level);
        uint8_t g = (uint8_t)(((wave_color >> 8) & 0xFF) * 0.7f * light_level);
        uint8_t b = (uint8_t)((wave_color & 0xFF) * 0.7f * light_level);
        led_strip_set_pixel(led_strip, center + 1, r, g, b);
    }
    
//最外侧LED（亮度最低）
    if (center - 2 >= 0 && center - 2 < LIGHT_STRENGTH) {
        uint8_t r = (uint8_t)(((wave_color >> 16) & 0xFF) * 0.4f * light_level);
        uint8_t g = (uint8_t)(((wave_color >> 8) & 0xFF) * 0.4f * light_level);
        uint8_t b = (uint8_t)((wave_color & 0xFF) * 0.4f * light_level);
        led_strip_set_pixel(led_strip, center - 2, r, g, b);
    }
    
    if (center + 2 >= 0 && center + 2 < LIGHT_STRENGTH) {
        uint8_t r = (uint8_t)(((wave_color >> 16) & 0xFF) * 0.4f * light_level);
        uint8_t g = (uint8_t)(((wave_color >> 8) & 0xFF) * 0.4f * light_level);
        uint8_t b = (uint8_t)((wave_color & 0xFF) * 0.4f * light_level);
        led_strip_set_pixel(led_strip, center + 2, r, g, b);
    }

    led_strip_refresh(led_strip);
    //ESP_LOGI(TAG, "水波位置: %d, 方向: %d", wave_position, wave_direction);
}


