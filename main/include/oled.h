#ifndef OLED_H
#define OLED_H
#include <stdint.h>
#include "esp_err.h"

void init_oled();
void init_floating_notes();
void create_floating_note(int key_num);
void update_floating_notes(void);
void update_music_animation_display(void);
esp_err_t ssd1306_clear_screen(uint8_t color);
esp_err_t ssd1306_refresh_gram(void);
esp_err_t display_fullscreen_simple(uint8_t *bitmap_data);

#endif