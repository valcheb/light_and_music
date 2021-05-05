#include "microphone.h"

#include "stm32f4xx_rcc.h"
#include "stm32f4xx_gpio.h"
#include "stm32f4xx_spi.h"
#include "misc.h"
#include "pdm_filter.h"

#include "stm32f4xx_dma.h"
#include "stm32f4xx_i2c.h"
#include <string.h>

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

#define STEREO_BUF_SIZE 2 * MIC_PCM_SIZE
static uint16_t stereo_buf[STEREO_BUF_SIZE];

#define ACCUM_TIME 1
#define ACCUM_BUF_SIZE STEREO_BUF_SIZE *ACCUM_TIME
static uint16_t mic_accum_buf[ACCUM_BUF_SIZE];

#define CODEC_TX_SIZE ACCUM_BUF_SIZE
static uint16_t codec_tx_buf[CODEC_TX_SIZE];

static bool rx_ready = false;
void set_rx_ready(bool state)
{
    rx_ready = state;
}

bool get_rx_ready()
{
    return rx_ready;
}

void rx_handler()
{
    PDM_Filter_64_LSB((uint8_t *)mic_pdm_buf, mic_pcm_buf, MIC_GAIN, &pdm_filter_init);
    for (int i = 0; i < STEREO_BUF_SIZE; i++)
    {
        stereo_buf[i] = mic_pcm_buf[i >> 1]; //stereo
    }

    static int buf_pos = 0;
    memcpy(mic_accum_buf + buf_pos * 2 * MIC_PCM_SIZE, stereo_buf, 2 * MIC_PCM_SIZE * sizeof(uint16_t));
    buf_pos++;
    if (buf_pos == ACCUM_TIME)
    {
        memcpy(codec_tx_buf, mic_accum_buf, ACCUM_BUF_SIZE * sizeof(uint16_t));
        buf_pos = 0;
        while (DMA1_Stream5->NDTR != 0)
        {
        };
        SPI_I2S_ITConfig(SPI2, SPI_I2S_IT_RXNE, ENABLE);
    }
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
#ifdef PLAYBACK_LOOP
            rx_ready = true;
#else
            rx_handler();
#endif
        }
    }
}

//playback test
static uint32_t i2c_read_register(uint8_t reg)
{
    uint32_t result = 0;

    /*!< While the bus is busy */
    while (I2C_GetFlagStatus(I2C1, I2C_FLAG_BUSY))
    {
    };

    /* Start the config sequence */
    I2C_GenerateSTART(I2C1, ENABLE);

    /* Test on EV5 and clear it */
    while (!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_MODE_SELECT))
    {
    };

    /* Transmit the slave address and enable writing operation */
    I2C_Send7bitAddress(I2C1, 0x94, I2C_Direction_Transmitter);

    /* Test on EV6 and clear it */
    while (!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED))
    {
    };

    /* Transmit the register address to be read */
    I2C_SendData(I2C1, reg);

    /* Test on EV8 and clear it */
    while (I2C_GetFlagStatus(I2C1, I2C_FLAG_BTF) == RESET)
    {
    };

    /*!< Send START condition a second time */
    I2C_GenerateSTART(I2C1, ENABLE);

    /*!< Test on EV5 and clear it (cleared by reading SR1 then writing to DR) */
    while (!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_MODE_SELECT))
    {
    };

    /*!< Send Codec address for read */
    I2C_Send7bitAddress(I2C1, 0x94, I2C_Direction_Receiver);

    /* Wait on ADDR flag to be set (ADDR is still not cleared at this level */
    while (I2C_GetFlagStatus(I2C1, I2C_FLAG_ADDR) == RESET)
    {
    };

    /*!< Disable Acknowledgment */
    I2C_AcknowledgeConfig(I2C1, DISABLE);

    /* Clear ADDR register by reading SR1 then SR2 register (SR1 has already been read) */
    (void)I2C1->SR2;

    /*!< Send STOP Condition */
    I2C_GenerateSTOP(I2C1, ENABLE);

    /* Wait for the byte to be received */
    while (I2C_GetFlagStatus(I2C1, I2C_FLAG_RXNE) == RESET)
    {
    };

    /*!< Read the byte received from the Codec */
    result = I2C_ReceiveData(I2C1);

    /* Wait to make sure that STOP flag has been cleared */
    while (I2C1->CR1 & I2C_CR1_STOP)
    {
    };

    /*!< Re-Enable Acknowledgment to be ready for another reception */
    I2C_AcknowledgeConfig(I2C1, ENABLE);

    /* Clear AF flag for next communication */
    I2C_ClearFlag(I2C1, I2C_FLAG_AF);

    /* Return the byte read from Codec */
    return result;
}

static uint32_t i2c_write_register(uint8_t reg, uint8_t data)
{
    /*!< While the bus is busy */
    while (I2C_GetFlagStatus(I2C1, I2C_FLAG_BUSY))
    {
    };

    /* Start the config sequence */
    I2C_GenerateSTART(I2C1, ENABLE);

    /* Test on EV5 and clear it */
    while (!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_MODE_SELECT))
    {
    };

    /* Transmit the slave address and enable writing operation */
    I2C_Send7bitAddress(I2C1, 0x94, I2C_Direction_Transmitter);

    /* Test on EV6 and clear it */
    while (!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED))
    {
    };

    /* Transmit the first address for write operation */
    I2C_SendData(I2C1, reg);

    /* Test on EV8 and clear it */
    while (!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_BYTE_TRANSMITTING))
    {
    };

    /* Prepare the register value to be sent */
    I2C_SendData(I2C1, data);

    /*!< Wait till all data have been physically transferred on the bus */
    while (!I2C_GetFlagStatus(I2C1, I2C_FLAG_BTF))
    {
    };

    /* End the configuration sequence */
    I2C_GenerateSTOP(I2C1, ENABLE);

    return (i2c_read_register(reg) == data) ? 0 : 1;
}

void playback_init()
{
    //gpio
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOD, ENABLE);
    RCC_PLLI2SCmd(ENABLE);

    GPIO_InitTypeDef gpio_init;

    //codec reset
    gpio_init.GPIO_Pin = GPIO_Pin_4;
    gpio_init.GPIO_Mode = GPIO_Mode_OUT;
    gpio_init.GPIO_OType = GPIO_OType_PP;
    gpio_init.GPIO_PuPd = GPIO_PuPd_NOPULL;
    gpio_init.GPIO_Speed = GPIO_Speed_25MHz;
    GPIO_Init(GPIOD, &gpio_init);

    //I2C1 pins
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);

    gpio_init.GPIO_Pin = GPIO_Pin_6 | GPIO_Pin_9;
    gpio_init.GPIO_Mode = GPIO_Mode_AF;
    gpio_init.GPIO_OType = GPIO_OType_OD;
    gpio_init.GPIO_PuPd = GPIO_PuPd_NOPULL;
    gpio_init.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOB, &gpio_init);

    GPIO_PinAFConfig(GPIOB, GPIO_PinSource6, GPIO_AF_I2C1);
    GPIO_PinAFConfig(GPIOB, GPIO_PinSource9, GPIO_AF_I2C1);

    //I2S3 pins
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC, ENABLE);

    gpio_init.GPIO_Pin = GPIO_Pin_7 | GPIO_Pin_10 | GPIO_Pin_12;
    gpio_init.GPIO_Mode = GPIO_Mode_AF;
    gpio_init.GPIO_Speed = GPIO_Speed_50MHz;
    gpio_init.GPIO_OType = GPIO_OType_PP;
    gpio_init.GPIO_PuPd = GPIO_PuPd_NOPULL;
    GPIO_Init(GPIOC, &gpio_init);

    gpio_init.GPIO_Pin = GPIO_Pin_4;
    GPIO_Init(GPIOA, &gpio_init);

    GPIO_PinAFConfig(GPIOA, GPIO_PinSource4, GPIO_AF_SPI3);
    GPIO_PinAFConfig(GPIOC, GPIO_PinSource7, GPIO_AF_SPI3);
    GPIO_PinAFConfig(GPIOC, GPIO_PinSource10, GPIO_AF_SPI3);
    GPIO_PinAFConfig(GPIOC, GPIO_PinSource12, GPIO_AF_SPI3);

    GPIO_ResetBits(GPIOD, GPIO_Pin_4);

    //i2c
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_I2C1, ENABLE);

    I2C_InitTypeDef i2c_init;
    i2c_init.I2C_Mode = I2C_Mode_I2C;
    i2c_init.I2C_DutyCycle = I2C_DutyCycle_2;
    i2c_init.I2C_OwnAddress1 = 0x33;
    i2c_init.I2C_Ack = I2C_Ack_Enable;
    i2c_init.I2C_AcknowledgedAddress = I2C_AcknowledgedAddress_7bit;
    i2c_init.I2C_ClockSpeed = 100000;
    I2C_Init(I2C1, &i2c_init);

    //i2s
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_SPI3, ENABLE);

    I2S_InitTypeDef I2S_InitStructure;
    I2S_InitStructure.I2S_AudioFreq = I2S_AudioFreq_8k;
    I2S_InitStructure.I2S_MCLKOutput = I2S_MCLKOutput_Enable;
    I2S_InitStructure.I2S_DataFormat = I2S_DataFormat_16b;
    I2S_InitStructure.I2S_Mode = I2S_Mode_MasterTx;
    I2S_InitStructure.I2S_Standard = I2S_Standard_Phillips;
    I2S_InitStructure.I2S_CPOL = I2S_CPOL_Low;
    I2S_Init(SPI3, &I2S_InitStructure);

    //i2s dma
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_DMA1, ENABLE);

    DMA_InitTypeDef DMA_InitStruct;
    DMA_Cmd(DMA1_Stream5, DISABLE);
    DMA_DeInit(DMA1_Stream5);

    DMA_InitStruct.DMA_Channel = DMA_Channel_0;
    DMA_InitStruct.DMA_PeripheralBaseAddr = (uint32_t)(&(SPI3->DR));
    DMA_InitStruct.DMA_Memory0BaseAddr = (uint32_t)codec_tx_buf;
    DMA_InitStruct.DMA_DIR = DMA_DIR_MemoryToPeripheral;
    DMA_InitStruct.DMA_BufferSize = CODEC_TX_SIZE;
    DMA_InitStruct.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
    DMA_InitStruct.DMA_MemoryInc = DMA_MemoryInc_Enable;
    DMA_InitStruct.DMA_PeripheralDataSize = DMA_PeripheralDataSize_HalfWord;
    DMA_InitStruct.DMA_MemoryDataSize = DMA_MemoryDataSize_HalfWord;
    DMA_InitStruct.DMA_Mode = DMA_Mode_Normal;
    DMA_InitStruct.DMA_Priority = DMA_Priority_High;
    DMA_InitStruct.DMA_FIFOMode = DMA_FIFOMode_Disable;
    DMA_InitStruct.DMA_FIFOThreshold = DMA_FIFOThreshold_1QuarterFull;
    DMA_InitStruct.DMA_MemoryBurst = DMA_MemoryBurst_Single;
    DMA_InitStruct.DMA_PeripheralBurst = DMA_PeripheralBurst_Single;
    DMA_Init(DMA1_Stream5, &DMA_InitStruct);

    //dma irq
    NVIC_InitTypeDef NVIC_InitStructure;

    //NVIC_PriorityGroupConfig(NVIC_PriorityGroup_3);
    NVIC_InitStructure.NVIC_IRQChannel = DMA1_Stream5_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 10;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 10;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);

    //CS43L22
    I2C_Cmd(I2C1, ENABLE);
    GPIO_ResetBits(GPIOD, GPIO_Pin_4);
    int n = 0xFFFF;
    while (n)
    {
        n--;
    }
    GPIO_SetBits(GPIOD, GPIO_Pin_4);

    /* Keep Codec powered OFF */
    uint32_t res = i2c_write_register(0x02, 0x01);
    res = i2c_write_register(0x04, 0xAF); /* SPK always OFF & HP always ON */
    /* Clock configuration: Auto detection */
    res = i2c_write_register(0x05, 0x81);
    /* Set the Slave Mode and the audio Standard */
    res = i2c_write_register(0x06, 0x04);
    /* Set the Master volume */
    int Volume = 80; //привести к адекватному виду TODOт
    int VV = ((Volume > 100) ? 100 : ((uint8_t)((Volume * 255) / 100)));
    if (VV > 0xE6)
    {
        /* Set the Master volume */
        res = i2c_write_register(0x20, VV - 0xE7);
        res = i2c_write_register(0x21, VV - 0xE7);
    }
    else
    {
        /* Set the Master volume */
        res = i2c_write_register(0x20, VV + 0x19);
        res = i2c_write_register(0x21, VV + 0x19);
    }

    /* Power on the Codec */
    res = i2c_write_register(0x02, 0x9E);
    /* Disable the analog soft ramp */
    res = i2c_write_register(0x0A, 0x00);
    /* Disable the digital soft ramp */
    res = i2c_write_register(0x0E, 0x04);
    /* Disable the limiter attack level */
    res = i2c_write_register(0x27, 0x00);
    /* Adjust Bass and Treble levels */
    res = i2c_write_register(0x1F, 0x0F);
    /* Adjust PCM volume level */
    res = i2c_write_register(0x1A, 0x0A);
    res = i2c_write_register(0x1B, 0x0A);
}

void playback_enable()
{
    I2S_Cmd(SPI3, ENABLE);
    SPI_I2S_DMACmd(SPI3, SPI_I2S_DMAReq_Tx, ENABLE);
    DMA_ITConfig(DMA1_Stream5, DMA_IT_TC, ENABLE);
    DMA_Cmd(DMA1_Stream5, ENABLE);
}

void DMA1_Stream5_IRQHandler(void)
{
    if (DMA_GetFlagStatus(DMA1_Stream5, DMA_FLAG_TCIF5) != RESET)
    {
        DMA_Cmd(DMA1_Stream5, DISABLE);
        DMA_ClearFlag(DMA1_Stream5, DMA_FLAG_TCIF5);
        DMA_Cmd(DMA1_Stream5, ENABLE);
    }
}

volatile uint32_t systick_clock = 0;

void systick_init()
{
    RCC_ClocksTypeDef RCC_Clocks;
    RCC_GetClocksFreq(&RCC_Clocks);
    SysTick_Config(RCC_Clocks.HCLK_Frequency / 1000000);
}

void systick_start_measure()
{
    systick_clock = 0;
}

uint32_t systick_stop_measure()
{
    return systick_clock;
}

void SysTick_Handler()
{
    systick_clock++;
}
