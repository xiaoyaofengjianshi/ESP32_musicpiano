/*
 * @file main.c
 * @brief 2025科中考核主程序文件
 * @author 何荣新
 * @date 2025-10-24
 * @version 1.0
 * 
 * @details 本文件是完整的考核项目
 */

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
#include "buzzer.h"
#include "oled.h"
#include "ttp229.h"
#include "ws2812.h"
#include "global.h"




//按键频率映射表，五音不全只能让AI生成对应频率TAT
int key_to_frequency[17] = {
    0,//0: 未使用
    311,//1: Re# (D#4)
    294,//2: Re (D4)
    277,//3: Do# (C#4)
    262,//4: Do (C4)
    0,//5: 音量增加/高音阶
    0,//6: 亮度增加
    0,//7: 音量减少/低音阶
    0,//8: 亮度减少
    415,//9: So# (G#4)
    440,//10: La (A4)
    466,//11: La# (A#4)
    494,//12: Si (B4)
    392,//13: So (G4)
    370,//14: Fa# (F#4)
    349,//15: Fa (F4)
    330//16: Mi (E4)
};

 menu_page_t current_menu_page = MENU_PAGE_MAIN;
 bool menu_mode = false;  // 是否处于菜单模式
 int64_t last_menu_action_time = 0;

char* current_base_scale = "C4";// 当前基础音阶
/*全局变量部分*/
char *TAG = "SSD1306_SPI";
bool is_playing_song = false;//是否正在播放歌曲
int current_song_index = 0;//当前播放的音符索引
int64_t note_start_time = 0;//当前音符开始时间（微秒）
bool note_playing = false;//当前是否有音符正在播放
int current_song_type = 0;//当前播放的歌曲类型：0-无，1-最炫民族风，2-夜空中最亮的星，剩下的后面再加
bool single_note_playing = false;//单键音符播放状态
int64_t single_note_start_time = 0;//单键音符开始时间
int single_note_frequency = 0;//单键音符频率
spi_device_handle_t spi;//SPI通信

void app_main() {
    /*初始化硬件*/
    light_start_set();//初始化LED灯带
    buzzer_init_pwm();// 初始化蜂鸣器PWM
    ttp229_init();// 初始化触摸按键    
    init_key_states();// 初始化按键状态
    init_oled();// 初始化OLED显示屏
    init_floating_notes();// 初始化音符动画系统
    /*初始化菜单相关变量*/
    current_menu_page = MENU_PAGE_MAIN;
    menu_mode = false;
    last_menu_action_time = esp_timer_get_time();
    current_base_scale = "C4";
    startup_rainbow();// 播放开机彩虹效果
    //ESP_LOGI(TAG, "系统初始化完成");

    // 主循环变量
    uint16_t key_state = 0;
    uint16_t last_key_state = 0;
    int64_t current_time = 0;
    int64_t last_display_update = esp_timer_get_time();
    
    // 按键状态标志
    bool key5_pressed = false;
    bool key6_pressed = false;
    bool key7_pressed = false;
    bool key8_pressed = false;

    // 主循环
    while (1) {
        // 读取当前按键状态
        key_state = ttp229_read_reversed();
        current_time = esp_timer_get_time();

        /*菜单导航按键*/
        
        // KEY7（A）
        if ((key_state & (1 << 6)) && !(last_key_state & (1 << 6))) {
            if (!key7_pressed && is_playing_song == false) {
                key7_pressed = true;
                menu_mode = true;
                last_menu_action_time = current_time;
                
                // 向前翻页
                current_menu_page = (current_menu_page - 1 + MENU_PAGE_COUNT) % MENU_PAGE_COUNT;
                if (current_menu_page == MENU_PAGE_COLOREGG) current_menu_page -=1;
                ESP_LOGI(TAG, "向前翻页到页面: %d", current_menu_page);
                
                // 停止所有播放
                stop_sound();
                is_playing_song = false;
                single_note_playing = false;
                note_playing = false;
            }
        } else if (!(key_state & (1 << 6)) && key7_pressed) {
            key7_pressed = false;
        }

        // KEY8（B）
        if ((key_state & (1 << 7)) && !(last_key_state & (1 << 7))) {
            if (!key8_pressed && is_playing_song == false) {
                key8_pressed = true;
                menu_mode = true;
                last_menu_action_time = current_time;
                
                // 向后翻页
                current_menu_page = (current_menu_page + 1) % MENU_PAGE_COUNT;
                ESP_LOGI(TAG, "向后翻页到页面: %d", current_menu_page);
                
                // 停止所有播放
                stop_sound();
                is_playing_song = false;
                single_note_playing = false;
                note_playing = false;
            }
        } else if (!(key_state & (1 << 7)) && key8_pressed) {
            key8_pressed = false;
        }

        /*放音界面功能按键*/
        // KEY5（E）
        if ((key_state & (1 << 4))) {
            if (!key5_pressed) {
                key5_pressed = true;
                menu_mode = true;
                last_menu_action_time = current_time;
                current_base_scale = "C3";
            }
            // 按住时降低音调
        } else if (!(key_state & (1 << 4)) && key5_pressed) {
            key5_pressed = false;
            current_base_scale = "C4";
            // 松开时恢复音调
        }

        // KEY6（L）
        if ((key_state & (1 << 5))) {
            if (!key6_pressed) {
                key6_pressed = true;
                menu_mode = true;
                last_menu_action_time = current_time;
                current_base_scale = "C5";
            }
            // 按住时升高音调
        } else if (!(key_state & (1 << 5)) && key6_pressed) {
            key6_pressed = false;
            current_base_scale = "C4";
            // 松开时恢复音调
        }

        /*非放音页面功能按键*/
        if ((key_state & (1 << 4)) && !(last_key_state & (1 << 4))) {
            // KEY5按下
            switch (current_menu_page) {
                case MENU_PAGE_TWINKLE:
                    // 播放/停止小星星
                    if (!is_playing_song) {
                        is_playing_song = true;
                        current_song_type = 1;
                        current_song_index = 0;
                        note_playing = false;
                        //ESP_LOGI(TAG, "开始播放《小星星》");
                    } else {
                        stop_sound();
                        is_playing_song = false;
                        note_playing = false;
                        //ESP_LOGI(TAG, "停止播放《小星星》");
                    }
                    break;
                    
                case MENU_PAGE_TIANXIA:
                    // 播放/停止天下相亲与相爱
                    if (!is_playing_song) {
                        is_playing_song = true;
                        current_song_type = 2;
                        current_song_index = 0;
                        note_playing = false;
                        ESP_LOGI(TAG, "开始播放《天下相亲与相爱》");
                    } else {
                        stop_sound();
                        is_playing_song = false;
                        note_playing = false;
                        ESP_LOGI(TAG, "停止播放《天下相亲与相爱》");
                    }
                    break;
                    
                case MENU_PAGE_BRIGHTNESS:
                    // 降低亮度
                    light_level -= 0.1f;
                    if (light_level < 0.0f) light_level = 0.0f;
                    ESP_LOGI(TAG, "亮度降低: %.0f%%", light_level * 100);
                    break;

                case MENU_PAGE_VOLUME:
                    // 降低音量
                    volume_level -= 0.1f;
                    if (volume_level < 0.0f) volume_level = 0.0f;
                    ESP_LOGI(TAG, "音量降低: %.0f%%", volume_level * 100);
                    break;

                default:
                    break;
            }
        }

        if ((key_state & (1 << 5)) && !(last_key_state & (1 << 5))) {
            // KEY6按下事件
            switch (current_menu_page) {
                case MENU_PAGE_TWINKLE:
                case MENU_PAGE_TIANXIA:
                    // 停止播放歌曲
                    stop_sound();
                    is_playing_song = false;
                    single_note_playing = false;
                    note_playing = false;
                    ESP_LOGI(TAG, "停止播放歌曲");
                    break;
                    
                case MENU_PAGE_BRIGHTNESS:
                    // 提高亮度
                    light_level += 0.1f;
                    if (light_level > 1.0f) light_level = 1.0f;
                    ESP_LOGI(TAG, "亮度提高: %.0f%%", light_level * 100);
                    break;
                
                case MENU_PAGE_YINFU:
                    break;
                
                case MENU_PAGE_VOLUME:
                    // 提高音量
                    volume_level += 0.1f;
                    if (volume_level > 1.0f) volume_level = 1.0f;
                    ESP_LOGI(TAG, "音量提高: %.0f%%", volume_level * 100);
                    break;
                
                case MENU_PAGE_COLOREGG:
                    break;

                default:
                    current_menu_page = MENU_PAGE_MAIN;
                    break;
            }
        }

        /*音符按键处理*/
        if (current_menu_page == MENU_PAGE_YINFU) {
            for (int i = 0; i < 16; i++) {
                int key_num = i + 1;
                int key_mask = (1 << i);

                // 跳过功能键
                if (key_num == 5 || key_num == 6 || key_num == 7 || key_num == 8) {
                    continue;
                }

                // 检查按键按下事件
                if ((key_state & key_mask) && !(last_key_state & key_mask)) {
                    // 更新LED状态
                    int led_index = key_to_led_index[key_num];
                    if (led_index >= 0 && led_index < LIGHT_STRENGTH) {
                        key_pressed[led_index] = true;
                    }

                    // 播放对应音符
                    int note_freq = key_to_frequency[key_num];
                    
                    if (note_freq > 0) {
                        // 已在播放其他音符时立即停止当前播放，重新开始防止让蜂鸣器发两个声出现bug
                        if (single_note_playing) {
                            stop_sound(); 
                        }
                        
                        // 音调改变
                        float temp_frequency_scale = 1.0f;
                        if (key5_pressed) {
                            temp_frequency_scale = 0.5f; // 降八度
                        } else if (key6_pressed) {
                            temp_frequency_scale = 2.0f; // 升八度
                        }
                        
                        note_freq = (int)(note_freq * temp_frequency_scale);
                        
                        start_sound(note_freq);
                        single_note_playing = true;
                        single_note_start_time = current_time;  // 重置放音时间
                        single_note_frequency = note_freq;
                        
                        // 显示音符名称
                        int note_index = key_to_note_name[key_num];
                        if (note_index != -1) {
                            current_note_display = note_index;
                            note_display_start_time = current_time;
                        }
                        
                        // 创建音符动画
                        create_floating_note(key_num);
                        
                        ESP_LOGI(TAG, "播放音符: 按键%d, 频率%dHz, 临时缩放系数: %.2f", 
                                key_num, note_freq, temp_frequency_scale);
                    }
                }
            }
        }
        // 抬起检测在所有页面都有效
        if (1) {
            for (int i = 0; i < 16; i++) {
                int key_num = i + 1;
                int key_mask = (1 << i);
                // 检查按键释放事件
                if (!(key_state & key_mask) && (last_key_state & key_mask)) {
                    // 更新LED状态
                    int led_index = key_to_led_index[key_num];
                    if (led_index >= 0 && led_index < LIGHT_STRENGTH) {
                        key_pressed[led_index] = false;
                    }
                    
                    
                }
            }
        }

        //音频播放更新
        update_audio_playback();

        //音符动画更新
        update_floating_notes();

        // 显示更新
        // 更新OLED显示
        if (current_time - last_display_update > 100000) {
            last_display_update = current_time;
            update_music_animation_display();
        }
        
        // LED效果更新
        if (is_playing_song) {
            moving_wave_effect();
        } else {
            rainbow(12);
        }

        // 保存当前按键状态
        last_key_state = key_state;
        
        // 短暂延迟以释放CPU资源
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
}