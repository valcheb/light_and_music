#include "lm.h"

#include <stdint.h>
#include <string.h>

#include "lm_config.h"
#include "pwm.h"
#include "microphone.h"
#include "player.h"
#include "spectrum.h"

static uint16_t sound_buffer[LM_SOUND_BUFFER_SIZE];
static uint16_t pcm_buffer[MIC_PCM_SIZE];
static float32_t coeff_buffer[LM_CHANNEL_NUMBER];

void lm_init()
{
    mic_init();
    pwm_error_init();
    pwm_init(LM_CHANNEL_NUMBER);
    spectrum_init();

    if (LM_SOUND_BUFFER_SIZE % MIC_PCM_SIZE != 0)
    {
        while (1)
        {
            pwm_error_iter();
        }
    }

    if (LM_USE_PLAYBACK)
    {
        player_init();
    }

}

void lm_enable()
{
    simple_leds_check();
    mic_enable();
    if (LM_USE_PLAYBACK)
    {
        player_enable();
    }
}

void lm_process()
{
    static int counter = 0;
    if (mic_rx_ready())
    {
        mic_rx_ready_clear();
        mic_pdm_pcm_convert(pcm_buffer);
        memcpy(sound_buffer + counter * MIC_PCM_SIZE, pcm_buffer, MIC_PCM_SIZE * sizeof(uint16_t));
        counter++;
    }

    if ( (counter * MIC_PCM_SIZE) == LM_SOUND_BUFFER_SIZE)
    {
        counter = 0;

        if (LM_USE_PLAYBACK)
        {
            player_send(sound_buffer, LM_SOUND_BUFFER_SIZE);
        }

        spectrum_analysis(sound_buffer, coeff_buffer);

        //test
        for (pwm_channel_e chan = PWM_CHANNEL_0; chan < LM_CHANNEL_NUMBER; chan++)
        {
            pwm_indicate(chan, btn_get_press_state(), coeff_buffer[(int)chan] * MAX_DUTY / BITCRUSH_DEPTH);
        }
    }
}
