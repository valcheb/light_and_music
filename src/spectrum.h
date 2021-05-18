#ifndef SPECTRUM_H_
#define SPECTRUM_H_

#include <stdint.h>
#include "arm_math.h"

#define BITCRUSH_DEPTH 64

/**
 * @brief Initialize functionality of spectrum analysis
 *
 */
void spectrum_init(void);

/**
 * @brief Find spectrum magnitude maximums of sound buffer in frequency ranges
 *
 * @param sound Pointer to sound buffer
 * @param coeff Pointer to resulting maximum's coefficients
 */
void spectrum_analysis(uint16_t *sound, uint16_t *coeff);

#endif /*SPECTRUM_H_*/
