#include "pwm.h"
#include "lm_config.h"

#define TIMER_PERIOD 1750 //48kHz PWM

typedef struct
{
    uint32_t     gpio_rcc;
    GPIO_TypeDef *gpio_port;
    uint16_t     gpio_pin;
    uint8_t      pin_source;
    uint8_t      gpio_af_tim;
    uint32_t     tim_rcc;
    TIM_TypeDef  *tim_base;
    int          tim_channel;
} pwm_channel_t;

typedef enum
{
    TIM_CHANNEL_1 = 1,
    TIM_CHANNEL_2 = 2,
    TIM_CHANNEL_3 = 3,
    TIM_CHANNEL_4 = 4,
} tim_channel_e;

const static pwm_channel_t pwm_error_led =
{
    RCC_AHB1Periph_GPIOA, GPIOA, GPIO_Pin_0, GPIO_PinSource0, GPIO_AF_TIM2, RCC_APB1Periph_TIM2, TIM2, TIM_CHANNEL_1
};

const static pwm_channel_t pwm_ch_pool[PWM_CHANNEL_ENUM_SIZE] =
{
    {RCC_AHB1Periph_GPIOD, GPIOD, GPIO_Pin_12, GPIO_PinSource12, GPIO_AF_TIM4, RCC_APB1Periph_TIM4, TIM4, TIM_CHANNEL_1},
    {RCC_AHB1Periph_GPIOD, GPIOD, GPIO_Pin_13, GPIO_PinSource13, GPIO_AF_TIM4, RCC_APB1Periph_TIM4, TIM4, TIM_CHANNEL_2},
    {RCC_AHB1Periph_GPIOD, GPIOD, GPIO_Pin_14, GPIO_PinSource14, GPIO_AF_TIM4, RCC_APB1Periph_TIM4, TIM4, TIM_CHANNEL_3},
    {RCC_AHB1Periph_GPIOD, GPIOD, GPIO_Pin_15, GPIO_PinSource15, GPIO_AF_TIM4, RCC_APB1Periph_TIM4, TIM4, TIM_CHANNEL_4}
};

static void pwm_ch_gpio_init(const pwm_channel_t *chan)
{
    GPIO_InitTypeDef gpio_init;

    RCC_AHB1PeriphClockCmd(chan->gpio_rcc, ENABLE);

    GPIO_StructInit(&gpio_init);
    gpio_init.GPIO_Mode = GPIO_Mode_AF;
    gpio_init.GPIO_OType = GPIO_OType_PP;
    gpio_init.GPIO_Pin = chan->gpio_pin;
    gpio_init.GPIO_Speed = GPIO_Speed_100MHz;
    GPIO_Init(chan->gpio_port, &gpio_init);
    GPIO_PinAFConfig(chan->gpio_port, chan->pin_source, chan->gpio_af_tim);
}

static void pwm_ch_timer_init(const pwm_channel_t *chan)
{
    TIM_TimeBaseInitTypeDef base_init;
    TIM_OCInitTypeDef oc_init;

    RCC_APB1PeriphClockCmd(chan->tim_rcc, ENABLE);

    TIM_TimeBaseStructInit(&base_init);
    base_init.TIM_Prescaler = 1 - 1;
    base_init.TIM_Period = TIMER_PERIOD - 1;
    base_init.TIM_CounterMode = TIM_CounterMode_Up;
    base_init.TIM_ClockDivision = TIM_CKD_DIV1;
    base_init.TIM_RepetitionCounter = 0;
    TIM_TimeBaseInit(chan->tim_base, &base_init);

    TIM_OCStructInit(&oc_init);
    oc_init.TIM_OCMode = TIM_OCMode_PWM1;
    oc_init.TIM_OutputState = TIM_OutputState_Enable;
    oc_init.TIM_Pulse = 0;
    oc_init.TIM_OCPolarity = TIM_OCPolarity_High;

    switch(chan->tim_channel) //NOTE not elegant
    {
        case 1:
        {
            TIM_OC1Init(chan->tim_base, &oc_init);
            TIM_OC1PreloadConfig(chan->tim_base, TIM_OCPreload_Enable);
            break;
        }
        case 2:
        {
            TIM_OC2Init(chan->tim_base, &oc_init);
            TIM_OC2PreloadConfig(chan->tim_base, TIM_OCPreload_Enable);
            break;
        }
        case 3:
        {
            TIM_OC3Init(chan->tim_base, &oc_init);
            TIM_OC3PreloadConfig(chan->tim_base, TIM_OCPreload_Enable);
            break;
        }
        case 4:
        {
            TIM_OC4Init(chan->tim_base, &oc_init);
            TIM_OC4PreloadConfig(chan->tim_base, TIM_OCPreload_Enable);
            break;
        }
    }
}

static void pwm_ch_enable(const pwm_channel_t *chan)
{
    TIM_ARRPreloadConfig(chan->tim_base, ENABLE);
    TIM_Cmd(chan->tim_base, ENABLE);
}

static void pwm_channel_init(pwm_channel_e i)
{
    const pwm_channel_t *chan = &pwm_ch_pool[i];

    pwm_ch_gpio_init(chan);
    pwm_ch_timer_init(chan);
    pwm_ch_enable(chan);
}

void pwm_init(int N)
{
    if (N > PWM_CHANNEL_ENUM_SIZE)
    {
        while(1)
        {
            pwm_error_iter();
        }
    }

    for (pwm_channel_e i = PWM_CHANNEL_0; i < N; i++)
    {
        pwm_channel_init(i);
    }
}

void pwm_error_init()
{
    pwm_ch_gpio_init(&pwm_error_led);
    pwm_ch_timer_init(&pwm_error_led);
    pwm_ch_enable(&pwm_error_led);
}

void pwm_error_iter()
{
    volatile uint32_t *ccr_ptr;
    switch(pwm_error_led.tim_channel)
    {
        case 1:
        {
            ccr_ptr = &pwm_error_led.tim_base->CCR1;
            break;
        }
        case 2:
        {
            ccr_ptr = &pwm_error_led.tim_base->CCR2;
            break;
        }
        case 3:
        {
            ccr_ptr = &pwm_error_led.tim_base->CCR3;
            break;
        }
        case 4:
        {
            ccr_ptr = &pwm_error_led.tim_base->CCR4;
            break;
        }
    }

    for (int duty = MIN_DUTY; duty < MAX_DUTY; duty++)
    {
        *ccr_ptr = duty * TIMER_PERIOD / MAX_DUTY;
    }

    for (int duty = MAX_DUTY; duty >= MIN_DUTY; duty--)
    {
        *ccr_ptr = duty * TIMER_PERIOD / MAX_DUTY;
    }
}

void pwm_set_duty_cycle(pwm_channel_e channel, uint16_t duty)
{
    const pwm_channel_t *chan = &pwm_ch_pool[channel];

    switch(chan->tim_channel)
    {
        case TIM_CHANNEL_1:
        {
            chan->tim_base->CCR1 = duty * TIMER_PERIOD / MAX_DUTY;
            break;
        }
        case TIM_CHANNEL_2:
        {
            chan->tim_base->CCR2 = duty * TIMER_PERIOD / MAX_DUTY;
            break;
        }
        case TIM_CHANNEL_3:
        {
            chan->tim_base->CCR3 = duty * TIMER_PERIOD / MAX_DUTY;
            break;
        }
        case TIM_CHANNEL_4:
        {
            chan->tim_base->CCR4 = duty * TIMER_PERIOD / MAX_DUTY;
            break;
        }
    }
}

void pwm_indicate(pwm_channel_e channel, lm_led_func_e led_func, uint16_t duty)
{
    switch(led_func)
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
        }
    }
}

void simple_leds_check()
{
    for (int i = 0; i < 5; i++)
    {
        for (int duty = MIN_DUTY; duty < MAX_DUTY; duty++)
        {
            for (pwm_channel_e chan = PWM_CHANNEL_0; chan < PWM_CHANNEL_ENUM_SIZE; chan++)
            {
                pwm_set_duty_cycle(chan, duty);
            }
        }

        for (int duty = MAX_DUTY; duty >= MIN_DUTY; duty--)
        {
            for (pwm_channel_e chan = PWM_CHANNEL_0; chan < PWM_CHANNEL_ENUM_SIZE; chan++)
            {
                pwm_set_duty_cycle(chan, duty);
            }
        }
    }
}
