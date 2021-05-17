#include "spectrum.h"
#include "lm_config.h"

#include "arm_const_structs.h"

#define IFFT_FLAG          0
#define DO_REVERSE_BIT     1
#define CHANNEL_BLOCK_SIZE LM_FFT_BUFFER_SIZE / 2 / LM_CHANNEL_NUMBER
#define NOISE_THRESHOLD    800
#define COMP_THRESHOLD     0.7079 * 0xffff
#define COMP_RATIO         3
#define AVER_CONST         0.8
#define EXPERIMENT_THRESHOLD  8000

static float32_t fft_input_buffer[2 * LM_FFT_BUFFER_SIZE];
static float32_t mag_output_buffer[LM_FFT_BUFFER_SIZE];
static const arm_cfft_instance_f32 *s;
static float32_t coeff_averange[LM_CHANNEL_NUMBER];
static float32_t hamming_window[LM_SOUND_BUFFER_SIZE];

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

    for (int i = 0; i < LM_SOUND_BUFFER_SIZE; i++)
    {
        hamming_window[i] = 0.54 - 0.46 * cos((2.0 * PI * (i - LM_SOUND_BUFFER_SIZE / 2) / (LM_SOUND_BUFFER_SIZE - 1)));
    }
}

void spectrum_analysis(uint16_t *sound, uint16_t *coeff)
{
    for (int i = 0; i < LM_FFT_BUFFER_SIZE; i++)
    {
        //hard knee simple sound compression
        if (sound[i] > COMP_THRESHOLD)
        {
            sound[i] = (uint16_t)(((float32_t)sound[i] - COMP_THRESHOLD) / COMP_RATIO + COMP_THRESHOLD);
        }

        //prepare fft buffer
        fft_input_buffer[2 * i] = (float32_t)sound[i] * hamming_window[i];
        fft_input_buffer[2 * i + 1] = 0;
    }

    //fft and mag
    arm_cfft_f32(s, fft_input_buffer, IFFT_FLAG, DO_REVERSE_BIT);
    arm_cmplx_mag_f32(fft_input_buffer, mag_output_buffer, LM_FFT_BUFFER_SIZE);

    //reduce impact of zero harmonic and hum
    mag_output_buffer[0] = 0.01 * mag_output_buffer[0];
    mag_output_buffer[1] = 0.01 * mag_output_buffer[1];

    for (int i = 0; i < LM_FFT_BUFFER_SIZE; i++)
    {
        //window width normalize
        mag_output_buffer[i] /= LM_FFT_BUFFER_SIZE;

        //simple noise gate
        if (mag_output_buffer[i] < NOISE_THRESHOLD)
        {
            mag_output_buffer[i] = 0;
        }
    }

    for (int i = 0; i < LM_CHANNEL_NUMBER; i++)
    {
        float32_t maxValue = 0.0;
        uint32_t maxIndex = 0;

        //find max in frequency range
        arm_max_f32(mag_output_buffer + CHANNEL_BLOCK_SIZE * i, CHANNEL_BLOCK_SIZE, &maxValue, &maxIndex);

        //exponential moving average
        coeff_averange[i] = maxValue * AVER_CONST + coeff_averange[i] * (1 - AVER_CONST);

        //hard crushing of bitdepth
        if (coeff_averange[i] > EXPERIMENT_THRESHOLD)
        {
            coeff_averange[i] = EXPERIMENT_THRESHOLD;
        }
        coeff[i] = (uint16_t)(coeff_averange[i] * BITCRUSH_DEPTH / EXPERIMENT_THRESHOLD);
    }
}
