#include "lm.h"

#include <stdint.h>
#include <string.h>

#include "microphone.h"

static int channel_number = 0;
static int buffer_size = 0;
static bool playback = false;
static int pcm_size = 0;
static uint16_t *sound_buffer;
static uint16_t *pcm_buffer;
static float32_t *magnitude_buffer;

static void lm_internal_init(int ch_number, int buf_size, bool pb)
{
    channel_number = ch_number;
    buffer_size = buf_size;
    pcm_size = MIC_PCM_SIZE;
    playback = pb;

    sound_buffer = (uint16_t *)malloc(buf_size * sizeof(uint16_t));
    if (sound_buffer == NULL)
    {
        while(1) {};
    }

    pcm_buffer = (uint16_t *)malloc(pcm_size * sizeof(uint16_t));
    if (pcm_buffer == NULL)
    {
        while(1) {};
    }

    magnitude_buffer = (float32_t *)malloc(channel_number * sizeof(float32_t));
    if (magnitude_buffer == NULL)
    {
        while(1) {};
    }
}

void lm_init(int ch_number, int buf_size, bool pb)
{
    lm_internal_init(ch_number, buf_size, pb);

    mic_init();
    //pwm_init(channel_number);

    //spectrum_init();

    /*
    if (playback)
    {
        playback_init();
    }
    */
}

void lm_process()
{
    static int counter = 0;
    if (mic_rx_ready())
    {
        mic_rx_ready_clear();
        mic_pdm_pcm_convert(pcm_buffer);
        memcpy(sound_buffer + counter * pcm_size, pcm_buffer, pcm_size);
        counter++;
    }

    if ( (counter * pcm_size) == buffer_size)
    {
        counter = 0;

        if (playback)
        {
            playback_send(sound_buffer);
        }

        spectral_analysis(sound_buffer, buffer_size,
                          magnitude_buffer, channel_number);

        pwm_indicate(magnitude_buffer, channel_number);
    }
}
