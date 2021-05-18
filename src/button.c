#include "button.h"
#include "stm32f4xx_rcc.h"
#include "stm32f4xx_gpio.h"

#define BTN_JITTER_COUNT 100

static bool btn_state = false;

void btn_delete_jitter()
{
    uint16_t counter = 0;
    bool old_state = GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_0);
    bool state;

    while (counter < BTN_JITTER_COUNT)
    {
        state = GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_0);

        if (state == old_state)
        {
            counter++;
        }
        else
        {
            counter--;
        }

        old_state = state;
    }
}

void btn_set_state(bool state)
{
    btn_state = state;
}

bool btn_get_state()
{
    return btn_state;
}

static void btn_gpio_init()
{
    GPIO_InitTypeDef GPIO_InitStructure;

    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);

    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_OD;
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
    GPIO_InitStructure.GPIO_Speed = GPIO_Low_Speed;
    GPIO_Init(GPIOA, &GPIO_InitStructure);
}

static void btn_exti_init()
{
    EXTI_InitTypeDef EXTI_InitStructure;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_SYSCFG, ENABLE);
    SYSCFG_EXTILineConfig(EXTI_PortSourceGPIOA, EXTI_PinSource0);

    EXTI_InitStructure.EXTI_Line = EXTI_Line0;
    EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
    EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising_Falling;
    EXTI_InitStructure.EXTI_LineCmd = ENABLE;
    EXTI_Init(&EXTI_InitStructure);
}

static void btn_irq_init()
{
    NVIC_InitTypeDef NVIC_InitStructure;

    NVIC_InitStructure.NVIC_IRQChannel = EXTI0_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);
}

void btn_init()
{
    btn_gpio_init();
    btn_exti_init();
    btn_irq_init();

    btn_set_state(false);
}

void EXTI0_IRQHandler(void)
{
    if (EXTI_GetITStatus(EXTI_Line0) != RESET)
    {
        if (GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_0))
        {
            btn_set_state(true);
        }
    }
    EXTI_ClearITPendingBit(EXTI_Line0);
}
