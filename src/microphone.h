#ifndef MICROPHONE_H_
#define MICROPHONE_H_

#include <stdbool.h>
#include <stdint.h>

#define MIC_GAIN     75
#define MIC_PDM_SIZE 64 //1ms for 16kHz input
#define MIC_PCM_SIZE 16 //pdm_filter_init.Fs == 16kHz / 1000

/**
 * @brief Initialize functionality of MP45DT02 microphone
 *
 */
void mic_init(void);

/**
 * @brief Enable functionality of MP45DT02 microphone
 *
 */
void mic_enable(void);

/**
 * @brief Get readiness of microphone receiver buffer for processing
 *
 * @return true  Buffer is ready
 * @return false Buffer is not ready
 */
bool mic_rx_ready(void);

/**
 * @brief Clear flag of readiness of microphone receiver buffer
 *
 */
void mic_rx_ready_clear(void);

/**
 * @brief Convert internal pdm buffer to external pcm buffer
 *
 * @param pcm_buffer Pointer to result of conversion
 */
void mic_pdm_pcm_convert(uint16_t *pcm_buffer);

#endif /*MICROPHONE_H_*/
