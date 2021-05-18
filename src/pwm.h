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

/**
 * @brief Initialize functionality of pwm led indication
 *
 * @param channel_number Number of leds (pwm channels)
 */
void pwm_init(int channel_number);

/**
 * @brief Initialize functionality of error led indication
 *
 */
void pwm_error_init(void);

/**
 * @brief Display led error indication
 *
 * Call in infinite loop
 *
 */
void pwm_error_iter(void);

/**
 * @brief Set duty cycle of pwm channel
 *
 * @param channel Pwm channel number
 * @param duty    Value of brightness
 */
void pwm_set_duty_cycle(pwm_channel_e channel, uint16_t duty);

/**
 * @brief Demonstration of leds serviceability
 *
 */
void simple_leds_check(void);

#endif /*PWM_H_*/
