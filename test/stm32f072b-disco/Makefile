OUTPUT = fw.elf

ROOT = ../..
PET = $(ROOT)/pet

INCLUDE_DIRS += $(ROOT)
INCLUDE_DIRS += $(ROOT)/net
INCLUDE_DIRS += $(ROOT)/test
INCLUDE_DIRS += $(ROOT)/scheduler
INCLUDE_DIRS += $(ROOT)/arch/gcc-cortex-m0
INCLUDE_DIRS += $(ROOT)/arch/gcc-cortex-common

ifeq ('x','')
endif

# Atomic list related tests.

SOURCES += $(ROOT)/test/LimitedCTree.cpp
SOURCES += $(ROOT)/test/SharedAtomicListSimple.cpp
SOURCES += $(ROOT)/test/SharedAtomicListReissue.cpp


# Mutex related tests.

SOURCES += $(ROOT)/test/MutexMany.cpp
SOURCES += $(ROOT)/test/MutexSingle.cpp
SOURCES += $(ROOT)/test/MutexDeadlock.cpp
SOURCES += $(ROOT)/test/MutexVsSelect.cpp
SOURCES += $(ROOT)/test/MutexSingleBusy.cpp
SOURCES += $(ROOT)/test/MutexPriorityInversion.cpp


# Semaphore tests.

SOURCES += $(ROOT)/test/Select.cpp
SOURCES += $(ROOT)/test/SemaphoreLock.cpp
SOURCES += $(ROOT)/test/SemaphorePrio.cpp
SOURCES += $(ROOT)/test/SemaphoreFromIrq.cpp
SOURCES += $(ROOT)/test/SemaphoreTimeout.cpp
SOURCES += $(ROOT)/test/SemaphoreCounting.cpp
SOURCES += $(ROOT)/test/SemaphorePassaround.cpp


# General task management.

SOURCES += $(ROOT)/test/Abort.cpp
SOURCES += $(ROOT)/test/Error.cpp
SOURCES += $(ROOT)/test/Sleep.cpp
SOURCES += $(ROOT)/test/Yield.cpp
SOURCES += $(ROOT)/test/TaskStart.cpp


# Policy sanity checking.

SOURCES += $(ROOT)/test/Policy.cpp
SOURCES += $(ROOT)/test/PolicyPrio.cpp


# Io management related tests.

SOURCES += $(ROOT)/test/Timeout.cpp
SOURCES += $(ROOT)/test/IoChannel.cpp
SOURCES += $(ROOT)/test/IoRequest.cpp


# Networking related tests.

SOURCES += $(ROOT)/test/NetSharedTable.cpp
SOURCES += $(ROOT)/test/NetBufferPool.cpp
SOURCES += $(ROOT)/test/NetPacket.cpp
SOURCES += $(ROOT)/test/NetChecksum.cpp
SOURCES += $(ROOT)/test/NetIpTransmission.cpp


# Testing utilities.

SOURCES += $(ROOT)/test/common/CommonTestUtils.cpp

SOURCES += $(ROOT)/arch/gcc-cortex-m0/Profile.cpp

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
SOURCES += st/stdperiph/src/stm32f0xx_tim.c
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
COMMONFLAGS += -g3 -Os
COMMONFLAGS += -fomit-frame-pointer 
COMMONFLAGS += -static

CXXFLAGS += $(COMMONFLAGS)
CXXFLAGS += -std=c++17
CXXFLAGS += -fno-rtti
CXXFLAGS += -fno-exceptions #-S

CFLAGS += $(COMMONFLAGS)
CFLAGS += -std=c11

LDFLAGS += -gc-sections
LDFLAGS += -nostartfiles
LDFLAGS += -Tstm32.ld

LIBS += gcc c

include ../toolchains/Compiler.arm.mk
include ../ultimate-makefile/Makefile.ultimate

enter:
	CD_TO=$(ROOT) make -C $(ROOT)/test/toolchains/ enter-the-matrix 
