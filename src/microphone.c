#include "microphone.h"

#include "stm32f4xx_rcc.h"
#include "stm32f4xx_gpio.h"
#include "stm32f4xx_spi.h"
#include "misc.h"
#include "pdm_filter.h"

static PDMFilter_InitStruct pdm_filter_init;
void mic_init()
{
    //gpio

    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC, ENABLE);

    GPIO_InitTypeDef gpio_init;
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

    // i2s over spi2
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_SPI2, ENABLE);
    RCC_PLLI2SCmd(ENABLE);

    I2S_InitTypeDef i2s_init;
    I2S_StructInit(&i2s_init);
    //SPI_I2S_DeInit(SPI2);
    i2s_init.I2S_AudioFreq = I2S_AudioFreq_16k; //TODO: study effect of different frequencies
    i2s_init.I2S_Standard = I2S_Standard_LSB;
    i2s_init.I2S_DataFormat = I2S_DataFormat_16b;
    i2s_init.I2S_CPOL = I2S_CPOL_High;
    i2s_init.I2S_Mode = I2S_Mode_MasterRx;
    i2s_init.I2S_MCLKOutput = I2S_MCLKOutput_Disable;
    I2S_Init(SPI2, &i2s_init);

    //i2s irq
    NVIC_InitTypeDef NVIC_InitStructure;
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_3);
    NVIC_InitStructure.NVIC_IRQChannel = SPI2_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);

    //pdm filter
    RCC_AHB1PeriphClockCmd(RCC_AHB1ENR_CRCEN, ENABLE);
    pdm_filter_init.LP_HZ = 0;
    pdm_filter_init.HP_HZ = 0;
    pdm_filter_init.Fs = 16000;
    pdm_filter_init.Out_MicChannels = 1;
    pdm_filter_init.In_MicChannels = 1;
    PDM_Filter_Init(&pdm_filter_init);
}

void mic_enable()
{
    I2S_Cmd(SPI2, ENABLE);
    SPI_I2S_ITConfig(SPI2, SPI_I2S_IT_RXNE, ENABLE);
}

#define MIC_PDM_SIZE 64 //1ms for 16kHz input
#define MIC_PCM_SIZE 16 //pdm_filter_init.Fs / 1000
#define MIC_GAIN 80
static uint16_t mic_pdm_buf[MIC_PDM_SIZE];
static uint16_t mic_pcm_buf[MIC_PCM_SIZE];

void SPI2_IRQHandler(void)
{
    static int mic_counter = 0;
    if (SPI_GetITStatus(SPI2, SPI_I2S_IT_RXNE) != RESET)
    {
        mic_pdm_buf[mic_counter++] = HTONS(SPI_I2S_ReceiveData(SPI2));

        if (mic_counter == MIC_PDM_SIZE)
        {
            mic_counter = 0;
            PDM_Filter_64_LSB((uint8_t *)mic_pdm_buf, mic_pcm_buf, MIC_GAIN, &pdm_filter_init);
        }
    }
}
