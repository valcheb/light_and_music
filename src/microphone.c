#include "microphone.h"

#include "stm32f4xx_rcc.h"
#include "stm32f4xx_gpio.h"
#include "stm32f4xx_spi.h"
#include "misc.h"
#include "pdm_filter.h"

static PDMFilter_InitStruct pdm_filter_init;

static void mic_gpio_init()
{
    GPIO_InitTypeDef gpio_init;

    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC, ENABLE);

    GPIO_StructInit(&gpio_init);
    gpio_init.GPIO_Mode = GPIO_Mode_AF;
    gpio_init.GPIO_OType = GPIO_OType_PP;
    gpio_init.GPIO_PuPd = GPIO_PuPd_NOPULL;
    gpio_init.GPIO_Speed = GPIO_Speed_50MHz;

    //SPI2 SCK
    gpio_init.GPIO_Pin = GPIO_Pin_10;
    GPIO_Init(GPIOB, &gpio_init);
    GPIO_PinAFConfig(GPIOB, GPIO_PinSource10, GPIO_AF_SPI2);

    //SPI2 MOSI
    gpio_init.GPIO_Pin = GPIO_Pin_3;
    GPIO_Init(GPIOC, &gpio_init);
    GPIO_PinAFConfig(GPIOC, GPIO_PinSource3, GPIO_AF_SPI2);
}

static void mic_i2s_init()
{
    I2S_InitTypeDef i2s_init;

    RCC_APB1PeriphClockCmd(RCC_APB1Periph_SPI2, ENABLE);
    RCC_PLLI2SCmd(ENABLE);

    I2S_StructInit(&i2s_init);
    //SPI_I2S_DeInit(SPI2);
    i2s_init.I2S_AudioFreq = I2S_AudioFreq_16k;
    i2s_init.I2S_Standard = I2S_Standard_LSB;
    i2s_init.I2S_DataFormat = I2S_DataFormat_16b;
    i2s_init.I2S_CPOL = I2S_CPOL_High;
    i2s_init.I2S_Mode = I2S_Mode_MasterRx;
    i2s_init.I2S_MCLKOutput = I2S_MCLKOutput_Disable;
    I2S_Init(SPI2, &i2s_init);
}

static void mic_i2s_irq_init()
{
    NVIC_InitTypeDef NVIC_InitStructure;

    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_3);
    NVIC_InitStructure.NVIC_IRQChannel = SPI2_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);
}

static void mic_pdm_filter_init()
{
    RCC_AHB1PeriphClockCmd(RCC_AHB1ENR_CRCEN, ENABLE);

    pdm_filter_init.LP_HZ = 0;
    pdm_filter_init.HP_HZ = 0;
    pdm_filter_init.Fs = 16000;
    pdm_filter_init.Out_MicChannels = 1;
    pdm_filter_init.In_MicChannels = 1;
    PDM_Filter_Init(&pdm_filter_init);
}

void mic_init()
{
    mic_gpio_init();
    mic_i2s_init();
    mic_i2s_irq_init();
    mic_pdm_filter_init();
}

void mic_enable()
{
    I2S_Cmd(SPI2, ENABLE);
    SPI_I2S_ITConfig(SPI2, SPI_I2S_IT_RXNE, ENABLE);
}

static uint16_t mic_pdm_buf[MIC_PDM_SIZE];
void mic_pdm_pcm_convert(uint16_t *pcm_buffer)
{
    PDM_Filter_64_LSB((uint8_t *)mic_pdm_buf, pcm_buffer, MIC_GAIN, &pdm_filter_init);
}

static bool rx_ready = false;
bool mic_rx_ready()
{
    return rx_ready;
}

void mic_rx_ready_clear()
{
    rx_ready = false;
}

void SPI2_IRQHandler(void)
{
    static int mic_counter = 0;
    if (SPI_GetITStatus(SPI2, SPI_I2S_IT_RXNE) != RESET)
    {
        mic_pdm_buf[mic_counter++] = HTONS(SPI_I2S_ReceiveData(SPI2));

        if (mic_counter == MIC_PDM_SIZE)
        {
            mic_counter = 0;
            rx_ready = true;
        }
    }
}
