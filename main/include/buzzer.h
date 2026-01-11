#ifndef BUZZER_H
#define BUZZER_H

void buzzer_init_pwm(void);
void start_sound(int frequency);
void update_audio_playback();
void stop_sound();
#endif