#include "spectrum.h"
#include "lm_config.h"

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

void spectrum_analysis(uint16_t *sound, float32_t *coeff)
{
    const static uint8_t ifftFlag = 0;
    const static uint8_t doBitReverse = 1;
    const static int block_size = (LM_FFT_BUFFER_SIZE / 2) / LM_CHANNEL_NUMBER;
    const static float32_t noise_gate = 0.02;

    float32_t window_energy = 0.0;

    for (int i = 0; i < LM_FFT_BUFFER_SIZE; i++)
    {
        fft_input_buffer[2 * i] = (float32_t)sound[i] / (0xffff * 0.5) - 1; //place signal between -1 and 1
        fft_input_buffer[2 * i + 1] = 0;
    }

    arm_cfft_f32(s, fft_input_buffer, ifftFlag, doBitReverse);
    arm_cmplx_mag_f32(fft_input_buffer, mag_output_buffer, LM_FFT_BUFFER_SIZE);

    for (int i = 0; i < LM_FFT_BUFFER_SIZE; i++)
    {
        window_energy += mag_output_buffer[i] * mag_output_buffer[i];
    }
    window_energy /= LM_FFT_BUFFER_SIZE;

    for (int i = 0; i < LM_CHANNEL_NUMBER; i++)
    {
        /*
        float32_t maxValue = 0.0;
        uint32_t maxIndex = 0;
        arm_max_f32(mag_output_buffer + 1 + block_size * i, block_size, &maxValue, &maxIndex); //+1 - avoid zero harmonic
        coeff[i] = maxValue / window_energy - noise_gate;
        */

        float32_t block_energy = 0.0;
        for (int j = block_size * i; j < block_size * (i + 1); j++)
        {
            if (j == 0) //avoid zero harmonic
            {
                continue;
            }
            block_energy += mag_output_buffer[j] * mag_output_buffer[j];
        }
        block_energy /= LM_FFT_BUFFER_SIZE;
        coeff[i] = block_energy / window_energy - noise_gate;

        if (coeff[i] < 0)
        {
            coeff[i] = 0;
        }
    }

    //pwm_array[i] = (uint16_t)( (max_array[i] * 1.5) * 0xffff);
}
