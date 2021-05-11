#ifndef SPECTRUM_H_
#define SPECTRUM_H_

#include <stdint.h>
#include "arm_math.h"

void spectrum_init(void);
void spectrum_analysis(uint16_t *sound, float32_t *coeff);

#endif /*SPECTRUM_H_*/
