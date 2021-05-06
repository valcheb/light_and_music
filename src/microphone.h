#ifndef MICROPHONE_H_
#define MICROPHONE_H_

#include <stdbool.h>

#define MIC_GAIN     75
#define MIC_PDM_SIZE 64 //1ms for 16kHz input
#define MIC_PCM_SIZE 16 //pdm_filter_init.Fs == 16kHz / 1000

void mic_init(void);
void mic_enable(void);
bool mic_rx_ready(void);
void mic_pdm_pcm_convert(uint16_t *pcm_buffer);

#endif /*MICROPHONE_H_*/
