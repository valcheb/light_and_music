#include "pwm.h"

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

const static pwm_channel_t pwm_ch_pool[PWM_CHANNEL_ENUM_SIZE] =
{
    {RCC_AHB1Periph_GPIOD, GPIOD, GPIO_Pin_12, GPIO_PinSource12, GPIO_AF_TIM4, RCC_APB1Periph_TIM4, TIM4, 1},
    {RCC_AHB1Periph_GPIOD, GPIOD, GPIO_Pin_13, GPIO_PinSource13, GPIO_AF_TIM4, RCC_APB1Periph_TIM4, TIM4, 2},
    {RCC_AHB1Periph_GPIOD, GPIOD, GPIO_Pin_14, GPIO_PinSource14, GPIO_AF_TIM4, RCC_APB1Periph_TIM4, TIM4, 3},
    {RCC_AHB1Periph_GPIOD, GPIOD, GPIO_Pin_15, GPIO_PinSource15, GPIO_AF_TIM4, RCC_APB1Periph_TIM4, TIM4, 4}
};

static void pwm_channel_init(pwm_channel_e i)
{
    const pwm_channel_t *chan = &pwm_ch_pool[i];

    //gpio
    RCC_AHB1PeriphClockCmd(chan->gpio_rcc, ENABLE);

    GPIO_InitTypeDef gpio_init;
    GPIO_StructInit(&gpio_init);
    gpio_init.GPIO_Mode = GPIO_Mode_AF;
    gpio_init.GPIO_OType = GPIO_OType_PP;
    gpio_init.GPIO_Pin = chan->gpio_pin;
    gpio_init.GPIO_Speed = GPIO_Speed_100MHz;
    GPIO_Init(chan->gpio_port, &gpio_init);
    GPIO_PinAFConfig(chan->gpio_port, chan->pin_source, chan->gpio_af_tim);

    //timer
    RCC_APB1PeriphClockCmd(chan->tim_rcc, ENABLE);

    TIM_TimeBaseInitTypeDef base_init;
    TIM_TimeBaseStructInit(&base_init);

    base_init.TIM_Prescaler = 1 - 1;
    base_init.TIM_Period = TIMER_PERIOD - 1;
    base_init.TIM_CounterMode = TIM_CounterMode_Up;
    base_init.TIM_ClockDivision = TIM_CKD_DIV1;
    base_init.TIM_RepetitionCounter = 0;
    TIM_TimeBaseInit(chan->tim_base, &base_init);

    TIM_OCInitTypeDef oc_init;
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

    TIM_ARRPreloadConfig(chan->tim_base, ENABLE);
    TIM_Cmd(chan->tim_base, ENABLE);
}

void pwm_init(int N)
{
    if (N > PWM_CHANNEL_ENUM_SIZE)
    {
        while(1)
        {
        }; //error
    }

    for (pwm_channel_e i = PWM_CHANNEL_0; i < N; i++)
    {
        pwm_channel_init(i);
    }
}

void pwm_set_duty_cycle(pwm_channel_e channel, uint16_t duty)
{
    const pwm_channel_t *chan = &pwm_ch_pool[channel];

    switch(chan->tim_channel)
    {
        case 1:
        {
            chan->tim_base->CCR1 = duty * TIMER_PERIOD / MAX_DUTY;
            break;
        }
        case 2:
        {
            chan->tim_base->CCR2 = duty * TIMER_PERIOD / MAX_DUTY;
            break;
        }
        case 3:
        {
            chan->tim_base->CCR3 = duty * TIMER_PERIOD / MAX_DUTY;
            break;
        }
        case 4:
        {
            chan->tim_base->CCR4 = duty * TIMER_PERIOD / MAX_DUTY;
            break;
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
