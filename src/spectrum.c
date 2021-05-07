#include "spectrum.h"
#include "lm_config.h"

#include "arm_math.h"
#include "arm_const_structs.h"

static float32_t fft_input_buffer[2 * LM_FFT_BUFFER_SIZE];
static float32_t mag_output_buffer[LM_FFT_BUFFER_SIZE];
static const arm_cfft_instance_f32 *s;

void spectrum_init()
{
    switch(LM_SOUND_BUFFER_SIZE)
    {
        case LM_SOUND_SIZE_64:
        {
            s = &arm_cfft_sR_f32_len64;
            break;
        }
        case LM_SOUND_SIZE_256:
        {
            s = &arm_cfft_sR_f32_len256;
            break;
        }
        case LM_SOUND_SIZE_1024:
        {
            s = &arm_cfft_sR_f32_len1024;
            break;
        }
    }
}

void spectrum_analysis(uint16_t *sound)
{
    for (int i = 0; i < LM_FFT_BUFFER_SIZE; i++)
    {
        fft_input_buffer[2 * i] = (float32_t)( (float32_t)sound[i]);
        fft_input_buffer[2 * i + 1] = 0;
    }

    uint8_t ifftFlag = 0;
    uint8_t doBitReverse = 1;
    arm_cfft_f32(s, fft_input_buffer, ifftFlag, doBitReverse);
    arm_cmplx_mag_f32(fft_input_buffer, mag_output_buffer, LM_FFT_BUFFER_SIZE);

    float32_t maxValue;
    uint32_t maxIndex;
    arm_max_f32(mag_output_buffer + 1, LM_FFT_BUFFER_SIZE - 1, &maxValue, &maxIndex); //+1 - avoid zero harmonic
}
