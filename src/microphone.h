#ifndef MICROPHONE_H_
#define MICROPHONE_H_

#include <stdint.h>
#include <stdbool.h>

void mic_init();
void mic_enable();

//#define PLAYBACK_LOOP 1

void playback_init(void);
void playback_enable(void);
void rx_handler(void);

void systick_init(void);
void systick_start_measure(void);
uint32_t systick_stop_measure(void);

void set_rx_ready(bool state);
bool get_rx_ready(void);

#endif /*MICROPHONE_H_*/
