#ifndef SPECTRUM_H_
#define SPECTRUM_H_

#include <stdint.h>
#include "arm_math.h"

#define BITCRUSH_DEPTH 64

void spectrum_init(void);
void spectrum_analysis(uint16_t *sound, uint16_t *coeff);

#endif /*SPECTRUM_H_*/
