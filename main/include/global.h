#ifndef GLOBAL_H
#define GLOBAL_H

/*引脚相关配置(简单对齐了一下，好看多了)*/
#define MY_SPI_HOST     SPI2_HOST
#define SPI_MOSI_GPIO   GPIO_NUM_17//出入
#define SPI_CLK_GPIO    GPIO_NUM_4//时钟
#define SPI_CS_GPIO     GPIO_NUM_19//片选(电平)
#define DC_GPIO         GPIO_NUM_18//数据与命令
#define RESET_GPIO      GPIO_NUM_5
#define BUZZER_GPIO     GPIO_NUM_25//蜂鸣器
#define TTP229_SCL_PIN  GPIO_NUM_27//TTP229时钟
#define TTP229_SDO_PIN  GPIO_NUM_26//TTP229输出

#define MAX_FLOATING_NOTES 10//最大同时显示的音符数量
#define LIGHT_STRENGTH  12//灯组长
#define LIGHT_GPIO      GPIO_NUM_2//灯组IO口


/*OLED屏幕参数*/
#define SSD1306_WIDTH       128
#define SSD1306_HEIGHT      64
#define SSD1306_BUFFER_SIZE (SSD1306_WIDTH * SSD1306_HEIGHT / 8)//缓冲区
#define SSD1306_PAGES       (SSD1306_HEIGHT / 8)//定义页数(为什么不叫行数呢，怪)



/*全局变量声明部分*/
extern char *TAG;
extern bool is_playing_song ;//是否正在播放歌曲
extern int current_song_index ;//当前播放的音符索引
extern int64_t note_start_time ;//当前音符开始时间（微秒）
extern bool note_playing ;//当前是否有音符正在播放
extern int current_song_type ;//当前播放的歌曲类型：0-无，1-最炫民族风，2-夜空中最亮的星，剩下的后面再加
extern bool single_note_playing ;//单键音符播放状态
extern int64_t single_note_start_time ;//单键音符开始时间
extern int single_note_frequency ;//单键音符频率
extern spi_device_handle_t spi;//SPI通信
extern uint8_t oled_buffer[SSD1306_BUFFER_SIZE];//OLED预显示(缓冲区)
extern float light_level;
extern float volume_level;
extern int key_to_led_index[];
extern int key_to_note_name[];
extern int current_note_display;
extern int64_t note_display_start_time;
extern bool key_pressed[];

// 菜单相关变量
typedef enum {
    MENU_PAGE_MAIN = 0,      // 主页面：按键播放
    MENU_PAGE_YINFU,        // 音符页面：音符播放
    MENU_PAGE_TWINKLE,       // 小星星播放
    MENU_PAGE_TIANXIA,       // 天下相亲与相爱播放
    MENU_PAGE_BRIGHTNESS,    // 亮度调节
    MENU_PAGE_VOLUME,        // 音量调节
    MENU_PAGE_COLOREGG,       // 菜单
    MENU_PAGE_COUNT          // 菜单页面总数
} menu_page_t;

extern menu_page_t current_menu_page;
extern bool menu_mode;
extern char* current_base_scale;
extern int twinkle_length;
extern int tianxia_length;
extern uint8_t fullscreen_bitmap[SSD1306_BUFFER_SIZE];
#endif