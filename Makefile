#
# Makefile for stm32f407 for wavetable_synthesis
#

.PHONY: all clean

#Instruments
CC=arm-none-eabi-gcc
OBJCOPY=arm-none-eabi-objcopy

#Linker script for mcu
LDSCRIPT := third_party/mcu/stm32f4_flash.ld

#Project name
TARGET := lm

#Build paths
BUILD := build

#Requared paths
SOURCED := . third_party/mcu third_party/PDM_filter src
CMSISD := third_party/CMSIS

#Preparation
#SOURCED := $(addprefix src/, $(SOURCES))

SOURCEF := $(wildcard $(addsuffix /*.c, $(SOURCED)))
OBJF := $(notdir $(SOURCEF))
OBJF := $(OBJF:.c=.o)
OBJF := $(addprefix $(BUILD)/,$(OBJF))

SUF := $(wildcard $(addsuffix /*.s, $(SOURCED)))
SUOBJF := $(notdir $(SUF))
SUOBJF := $(SUOBJF:.s=.o)
SUOBJF := $(addprefix $(BUILD)/,$(SUOBJF))

#Compile Flags
CFLAGS  := -g -Wall -Wno-missing-braces -std=c99
CFLAGS += -Os
CFLAGS += -flto
CFLAGS += -mthumb -mcpu=cortex-m4
CFLAGS += -mfloat-abi=softfp -DARM_MATH_CM4=1 -D__FPU_PRESENT=1 -mfpu=fpv4-sp-d16
CFLAGS += -I$(CMSISD)/include -I$(CMSISD)/device $(addprefix -I, $(SOURCED))

#Link Flags
LDFLAGS := -Wl,-Map,$(BUILD)/$(TARGET).map -g -T$(LDSCRIPT) --specs=nano.specs --specs=nosys.specs
LDFLAGS += -Lthird_party/PDM_filter -lPDMFilter_GCC -lm

#Headers of libraries
SPLD := third_party/SPL
CFLAGS += -I$(SPLD)/inc
#used lib source
OBJF += $(BUILD)/stm32f4xx_rcc.o $(BUILD)/stm32f4xx_gpio.o $(BUILD)/stm32f4xx_tim.o $(BUILD)/stm32f4xx_spi.o $(BUILD)/misc.o
OBJF += $(BUILD)/stm32f4xx_dma.o $(BUILD)/stm32f4xx_i2c.o $(BUILD)/stm32f4xx_exti.o $(BUILD)/stm32f4xx_syscfg.o

#DSP
DSPD := third_party/DSP
SUOBJF += $(BUILD)/arm_bitreversal2.o
OBJF += $(BUILD)/arm_const_structs.o $(BUILD)/arm_common_tables.o $(BUILD)/arm_cfft_f32.o
OBJF += $(BUILD)/arm_cfft_radix8_f32.o $(BUILD)/arm_cmplx_mag_f32.o $(BUILD)/arm_max_f32.o

#Paths for compilator
VPATH := $(SOURCED)
#path for lib source
VPATH += $(SPLD)/src $(DSPD)/Source/CommonTables $(DSPD)/Source/TransformFunctions $(DSPD)/Source/ComplexMathFunctions $(DSPD)/Source/StatisticsFunctions

#Rules
all: mkdir $(BUILD)/$(TARGET).elf $(BUILD)/$(TARGET).hex

mkdir:
	mkdir -p ./$(BUILD)

%.hex: %.elf
	echo $(CFLAGS)
	$(OBJCOPY) -O ihex $^ $@

$(BUILD)/%.elf: suobj obj
	$(CC) $(CFLAGS) -o $@ $(SUOBJF) $(OBJF) $(LDFLAGS)

suobj: $(SUOBJF)

$(BUILD)/%.o: %.s
	$(CC) -c -MD $(CFLAGS) -o $@ $^

obj: $(OBJF)

$(BUILD)/%.o: %.c
	$(CC) -c -MD $(CFLAGS) -o $@ $^

clean:
	rm -rf $(BUILD)

include $(wildcard *.d)
