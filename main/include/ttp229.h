#ifndef TTP229_H
#define TTP229_H
#include <stdint.h>

void ttp229_init();
void init_key_states();
uint16_t ttp229_read_reversed();
#endif
