#ifndef LIGHT_MUSIC_H_
#define LIGHT_MUSIC_H_

#include <stdbool.h>

void lm_init(int ch_number, int buf_size, int pcm_b_size, bool pb);
void lm_process(void);

#endif /*LIGHT_MUSIC_H_*/
