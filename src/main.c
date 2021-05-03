#include "pwm.h"
#include "microphone.h"

int main()
{
    //SystemInit() setups the system clock up to 168MHz
    //NOTE: gcc project seems to be slower than keil
    //systick_init();

    pwm_init(1);
    simple_leds_check();

    mic_init();
    playback_init();

    playback_enable();
    mic_enable();

    while (1)
    {
#ifdef PLAYBACK_LOOP
        if (get_rx_ready() == true)
        {
            set_rx_ready(false);
            rx_handler();
        }
#endif
    }

    return 0;
}
