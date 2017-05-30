OUTPUT = fw.elf

ROOT = ../../..
PET = $(ROOT)/../pet

INCLUDE_DIRS += $(ROOT)
INCLUDE_DIRS += $(ROOT)/test
INCLUDE_DIRS += $(ROOT)/ports/gcc/cortex-m0

SOURCES += $(wildcard $(ROOT)/test/*.cpp)

SOURCES += $(ROOT)/ports/gcc/cortex-m0/Profile.cpp

SOURCES += $(PET)/1test/TestRunner.cpp
SOURCES += $(PET)/1test/TraceOutput.cpp
SOURCES += $(PET)/1test/FailureInjector.cpp

SOURCES += main.cpp
SOURCES += cxx.cpp
SOURCES += startup.cpp
SOURCES += vectors.cpp
SOURCES += setjmp.cpp
SOURCES += semihosting.cpp

SOURCES += st/stdperiph/src/stm32f0xx_gpio.c
SOURCES += st/stdperiph/src/stm32f0xx_rcc.c
SOURCES += st/stdperiph/src/stm32f0xx_misc.c
SOURCES += st/stdperiph/src/stm32f0xx_dbgmcu.c

INCLUDE_DIRS += .
INCLUDE_DIRS += st/cmsis/Include
INCLUDE_DIRS += st/stdperiph/inc
INCLUDE_DIRS += st/cmsis/Device/ST/STM32F0xx/Include

INCLUDE_DIRS += $(PET)

COMMONFLAGS += -Wno-attributes
COMMONFLAGS += -DSTM32F072=1
COMMONFLAGS += -DUSE_STDPERIPH_DRIVER=1
COMMONFLAGS += -mcpu=cortex-m0 -mthumb 
COMMONFLAGS += -O1 -ggdb 
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

LIBS += gcc 

LIB_DIRS += $(GCCPATH)/arm-none-eabi/lib/armv6-m/ 
LIB_DIRS += $(GCCPATH)/lib/gcc/arm-none-eabi/5.4.1/armv6-m/  

include ../../ultimate-makefile/Makefile.ultimate
