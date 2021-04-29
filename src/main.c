#include "pwm.h"

int main()
{
    //SystemInit() setups the system clock up to 168MHz

    pwm_init(1);
    simple_leds_check();

    while (1)
    {
    };

    return 0;
}
