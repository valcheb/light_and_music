#include "lm.h"

#include <stdint.h>
#include <string.h>

#include "lm_config.h"
#include "pwm.h"
#include "microphone.h"
#include "player.h"
#include "spectrum.h"
#include "button.h"

static uint16_t sound_buffer[LM_SOUND_BUFFER_SIZE];
static int sound_buffer_wpos = 0;
static uint16_t pcm_buffer[MIC_PCM_SIZE];
static uint16_t coeff_buffer[LM_CHANNEL_NUMBER];
static lm_led_func_e led_func = LM_LED_FUNC_SMOOTH;

static void lm_indicate(pwm_channel_e channel, lm_led_func_e led_func, uint16_t duty)
{
    switch (led_func)
    {
        case LM_LED_FUNC_SMOOTH:
        {
            pwm_set_duty_cycle(channel, duty);
            break;
        }
        case LM_LED_FUNC_SHARP:
        {
            if (duty > MAX_DUTY / 1024)
            {
               pwm_set_duty_cycle(channel, MAX_DUTY);
            }
            else
            {
                pwm_set_duty_cycle(channel, MIN_DUTY);
            }
            break;
        }
    }
}

void lm_init()
{
    mic_init();
    pwm_error_init();
    pwm_init(LM_CHANNEL_NUMBER);
    spectrum_init();
    btn_init();

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
    if (mic_rx_ready())
    {
        mic_rx_ready_clear();
        mic_pdm_pcm_convert(pcm_buffer);
        memcpy(sound_buffer + sound_buffer_wpos * MIC_PCM_SIZE, pcm_buffer, MIC_PCM_SIZE * sizeof(uint16_t));
        sound_buffer_wpos++;
    }

    if (btn_get_state())
    {
        btn_delete_jitter();

        if (GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_0))
        {
            led_func++;
            if (led_func >= LM_LED_FUNC_ENUM_SIZE)
            {
                led_func = LM_LED_FUNC_SMOOTH;
            }
        }

        btn_set_state(false);
    }

    if ( (sound_buffer_wpos * MIC_PCM_SIZE) == LM_SOUND_BUFFER_SIZE)
    {
        sound_buffer_wpos = 0;

        if (LM_USE_PLAYBACK)
        {
            player_send(sound_buffer, LM_SOUND_BUFFER_SIZE);
        }

        spectrum_analysis(sound_buffer, coeff_buffer);

        for (pwm_channel_e chan = PWM_CHANNEL_0; chan < LM_CHANNEL_NUMBER; chan++)
        {
            lm_indicate(chan, led_func, coeff_buffer[(int)chan] * MAX_DUTY / BITCRUSH_DEPTH);
        }
    }
}
