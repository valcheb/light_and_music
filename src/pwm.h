#ifndef PWM_H_
#define PWM_H_

#include "stm32f4xx_rcc.h"
#include "stm32f4xx_gpio.h"
#include "stm32f4xx_tim.h"

#define MIN_DUTY 0
#define MAX_DUTY 0xFFFF

typedef enum
{
    PWM_CHANNEL_0 = 0,
    PWM_CHANNEL_1,
    PWM_CHANNEL_2,
    PWM_CHANNEL_3,
    PWM_CHANNEL_ENUM_SIZE
} pwm_channel_e;

void pwm_init(int N);
void pwm_error_init(void);
void pwm_error_iter(void);
void pwm_set_duty_cycle(pwm_channel_e channel, uint16_t duty);
void pwm_indicate(pwm_channel_e channel, lm_led_func_e led_func, uint16_t duty);
void simple_leds_check(void);

#endif /*PWM_H_*/
