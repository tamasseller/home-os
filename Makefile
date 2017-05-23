OUTPUT = fw.elf

SOURCES += cxx.cpp
SOURCES += main.cpp
SOURCES += startup.cpp
SOURCES += vectors.cpp

SOURCES += st/stdperiph/src/stm32f0xx_pwr.c
SOURCES += st/stdperiph/src/stm32f0xx_i2c.c
SOURCES += st/stdperiph/src/stm32f0xx_spi.c
SOURCES += st/stdperiph/src/stm32f0xx_iwdg.c
SOURCES += st/stdperiph/src/stm32f0xx_syscfg.c
SOURCES += st/stdperiph/src/stm32f0xx_exti.c
SOURCES += st/stdperiph/src/stm32f0xx_crs.c
SOURCES += st/stdperiph/src/stm32f0xx_tim.c
SOURCES += st/stdperiph/src/stm32f0xx_comp.c
SOURCES += st/stdperiph/src/stm32f0xx_gpio.c
SOURCES += st/stdperiph/src/stm32f0xx_rcc.c
SOURCES += st/stdperiph/src/stm32f0xx_can.c
SOURCES += st/stdperiph/src/stm32f0xx_rtc.c
SOURCES += st/stdperiph/src/stm32f0xx_wwdg.c
SOURCES += st/stdperiph/src/stm32f0xx_crc.c
SOURCES += st/stdperiph/src/stm32f0xx_dma.c
SOURCES += st/stdperiph/src/stm32f0xx_usart.c
SOURCES += st/stdperiph/src/stm32f0xx_dac.c
SOURCES += st/stdperiph/src/stm32f0xx_adc.c
SOURCES += st/stdperiph/src/stm32f0xx_misc.c
SOURCES += st/stdperiph/src/stm32f0xx_flash.c
SOURCES += st/stdperiph/src/stm32f0xx_dbgmcu.c
SOURCES += st/stdperiph/src/stm32f0xx_cec.c

INCLUDE_DIRS += .
INCLUDE_DIRS += st/cmsis/Include
INCLUDE_DIRS += st/stdperiph/inc
INCLUDE_DIRS += st/cmsis/Device/ST/STM32F0xx/Include

COMMONFLAGS += -Wno-attributes
COMMONFLAGS += -DSTM32F072=1
COMMONFLAGS += -DUSE_STDPERIPH_DRIVER=1
COMMONFLAGS += -mcpu=cortex-m0 -mthumb 
COMMONFLAGS += -O1 -g3 
COMMONFLAGS += -static

CXXFLAGS += $(COMMONFLAGS)
CXXFLAGS += -std=c++11 
CXXFLAGS += -fno-rtti
CXXFLAGS += -fno-exceptions #-S

CFLAGS += $(COMMONFLAGS)
CFLAGS += -std=c11

GCCPATH = /opt/gcc-arm-none-eabi-5_4-2016q3
GCCPREF = $(GCCPATH)/bin/arm-none-eabi-
CC = $(GCCPREF)gcc
CXX = $(GCCPREF)g++
LD = $(GCCPREF)ld

LDFLAGS += -gc-sections
LDFLAGS += -nostartfiles
LDFLAGS += -Tstm32.ld

LIBS += gcc c

LIB_DIRS += $(GCCPATH)/arm-none-eabi/lib/armv6-m/ 
LIB_DIRS += $(GCCPATH)/lib/gcc/arm-none-eabi/5.4.1/armv6-m/  

include ultimate-makefile/Makefile.ultimate
