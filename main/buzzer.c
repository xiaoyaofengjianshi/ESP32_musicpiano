/*
 * @file buzzer.c
 * @brief 2025科中考核无源蜂鸣器相关文件
 * @author 何荣新
 * 
 * @details 本文件主要实现了无源蜂鸣器发声与变调功能
 */

#include "buzzer.h"
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

//蜂鸣器
float volume_level = 0.1f;//音量

/*歌曲数据*/
//小星星 
int twinkle_notes[] = {
    262, 262, 392, 392, 440, 440, 392,//1155665
    349, 349, 330, 330, 294, 294, 262,//4433221
    392, 392, 349, 349, 330, 330, 294,//5544332
    392, 392, 349, 349, 330, 330, 294,//5544332
    262, 262, 392, 392, 440, 440, 392,//1155665
    349, 349, 330, 330, 294, 294, 262//4433221
};
int twinkle_durations[] = {
    500, 500, 500, 500, 500, 500, 1000,
    500, 500, 500, 500, 500, 500, 1000,
    500, 500, 500, 500, 500, 500, 1000,
    500, 500, 500, 500, 500, 500, 1000,
    500, 500, 500, 500, 500, 500, 1000,
    500, 500, 500, 500, 500, 500, 1000
};
//百分比数据，方便后面OLED进度条显示
int twinkle_length = sizeof(twinkle_notes) / sizeof(twinkle_notes[0]);
//天下相亲与相爱~
int tianxia_notes[] = {
    523, 587, 784, 659, 659, 587,
    523, 523, 783, 392, 493, 392,
    493, 493, 493, 523, 493, 466,
    587, 659, 783, 698, 698, 466,
    523, 523, 440, 392, 523, 392,
    523, 523, 587, 659, 659, 587,
    //回第一句
    523, 587, 784, 659, 659, 587,
    523, 523, 783, 392, 493, 392,
    493, 493, 493, 523, 493, 466,
    587, 659, 783, 698, 698, 466,
    523, 523, 440, 392, 523, 392,
    523, 523, 587, 659, 659, 587,
    0
};
int tianxia_durations[] = {
    1000, 800, 400, 400, 400, 200,     
    200, 800, 400, 400, 400, 200,      
    200, 400, 200, 200, 200, 600,      
    400, 400, 400, 400, 400, 200,
    200, 800, 400, 400, 400, 400,
    400, 400, 400, 400, 400, 600,   
    
    400, 400, 400, 400, 400, 200,     
    200, 800, 400, 400, 400, 200,      
    200, 400, 200, 200, 200, 600,      
    400, 400, 400, 400, 400, 200,
    200, 800, 400, 400, 400, 400,
    400, 400, 400, 400, 400, 600, 
    1000
};
//百分比
int tianxia_length = sizeof(tianxia_notes) / sizeof(tianxia_notes[0]);


//蜂鸣器PWM初始化,比着网上现成的写的
void buzzer_init_pwm(void) {
    ledc_timer_config_t timer_config = {
        .speed_mode = LEDC_LOW_SPEED_MODE,//定时器模式
        .duty_resolution = LEDC_TIMER_8_BIT,//分辨率
        .timer_num = LEDC_TIMER_0,
        .freq_hz = 1000,//基础频率
        .clk_cfg = LEDC_AUTO_CLK,
    };
    ledc_timer_config(&timer_config);

    ledc_channel_config_t channel_config = {
        .gpio_num = BUZZER_GPIO,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel = LEDC_CHANNEL_0,
        .intr_type = LEDC_INTR_DISABLE,
        .timer_sel = LEDC_TIMER_0,
        .duty = 0,
    };
    ledc_channel_config(&channel_config);
}

//开始播放声音
void start_sound(int frequency) {
    buzzer_init_pwm();
    
//应用音量控制
    int actual_duty = (int)(128 * volume_level);
//actual_duty = 0;在科中的时候静音防止扰民
    if (actual_duty <= 0) {
        return;//音量为0时直接静音
    }
    if (frequency <= 0) {
//如果是0或负数，直接静音并返回
        return;
    }
    ledc_set_freq(LEDC_LOW_SPEED_MODE, LEDC_TIMER_0, frequency);
    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, actual_duty);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
}


//停止播放声音
void stop_sound() {
    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, 0);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
}


void update_audio_playback() {
    int64_t current_time = esp_timer_get_time();
    
    // 单键音符播放
    if (single_note_playing) {
        if (current_time - single_note_start_time >= 200000) { // 200ms后停止
            stop_sound();
            single_note_playing = false;
        }
    }
    
    // 歌曲播放
    if (is_playing_song && !single_note_playing) {
        if (!note_playing) {
            // 播放新音符
            int note_freq = 0;
            if (current_song_type == 1) {
                note_freq = twinkle_notes[current_song_index];
            } else if (current_song_type == 2) {
                note_freq = tianxia_notes[current_song_index];
            }
            
            
            start_sound(note_freq);
            note_start_time = current_time;
            note_playing = true;
        } else {
            // 检查音符是否应该结束
            int note_duration = 0;
            if (current_song_type == 1) {
                note_duration = twinkle_durations[current_song_index];
            } else if (current_song_type == 2) {
                note_duration = tianxia_durations[current_song_index];
            }
            
            if (current_time - note_start_time >= note_duration * 1000) {
                stop_sound();
                note_playing = false;
                current_song_index++;
                
                // 检查歌曲是否播放完毕
                if ((current_song_type == 1 && current_song_index >= twinkle_length) ||
                    (current_song_type == 2 && current_song_index >= tianxia_length)) {
                    is_playing_song = false;
                    note_playing = false;
                    ESP_LOGI(TAG, "歌曲播放完毕");
                }
            }
        }
    }
}