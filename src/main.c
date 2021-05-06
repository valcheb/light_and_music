#include "lm.h"

int main()
{
    //SystemInit() setups the system clock up to 168MHz

    lm_init();
    lm_enable();

    while (1)
    {
        lm_process();
    }

    return 0;
}
