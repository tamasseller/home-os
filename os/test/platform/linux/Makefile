OUTPUT = test

ROOT = ../../..
PET = $(ROOT)/../pet

INCLUDE_DIRS += $(ROOT)
INCLUDE_DIRS += $(ROOT)/test
INCLUDE_DIRS += $(ROOT)/ports/gcc/linux-um

# Atomic list related tests.
#SOURCES += $(ROOT)/test/AtomicListSimple.cpp
#SOURCES += $(ROOT)/test/AtomicListReissue.cpp


# Mutex related tests.

#SOURCES += $(ROOT)/test/MutexMany.cpp
#SOURCES += $(ROOT)/test/MutexSingle.cpp
#SOURCES += $(ROOT)/test/MutexSingleBusy.cpp
SOURCES += $(ROOT)/test/MutexPriorityInversion.cpp


# Semaphore tests.

#SOURCES += $(ROOT)/test/SemaphorePassaround.cpp
#SOURCES += $(ROOT)/test/SemaphoreTimeout.cpp
#SOURCES += $(ROOT)/test/SemaphorePrio.cpp
#SOURCES += $(ROOT)/test/SemaphoreCounting.cpp
#SOURCES += $(ROOT)/test/SemaphoreFromIrq.cpp
SOURCES += $(ROOT)/test/Select.cpp


# General task management.

#SOURCES += $(ROOT)/test/Yield.cpp
#SOURCES += $(ROOT)/test/Sleep.cpp
#SOURCES += $(ROOT)/test/TaskStart.cpp


# Policy sanity checking.

SOURCES += $(ROOT)/test/Policy.cpp
SOURCES += $(ROOT)/test/PolicyPrio.cpp


# Testing utilities.

SOURCES += $(ROOT)/test/TestSuite.cpp
SOURCES += $(ROOT)/test/CommonTestUtils.cpp

SOURCES += $(ROOT)/ports/gcc/linux-um/Profile.cpp

SOURCES += $(PET)/1test/TestRunner.cpp
SOURCES += $(PET)/1test/TraceOutput.cpp
SOURCES += $(PET)/1test/FailureInjector.cpp

SOURCES += $(PET)/ubiquitous/PrintfWriter.cpp

SOURCES += main.cpp

INCLUDE_DIRS += .
INCLUDE_DIRS += $(PET)

CXXFLAGS += -std=c++11 
CXXFLAGS += -g3 
CXXFLAGS += -O1
CXXFLAGS += -fno-rtti
CXXFLAGS += -fno-exceptions 
CXXFLAGS += -pedantic 
CXXFLAGS += -Wall 
CXXFLAGS += -Wcomment
CXXFLAGS += -Wuninitialized
CXXFLAGS += -Winit-self
CXXFLAGS += -Wstrict-aliasing=2
CXXFLAGS += -Wextra
CXXFLAGS += -Wcast-align
CXXFLAGS += -Wdisabled-optimization
CXXFLAGS += -Wno-unused
CXXFLAGS += -Werror
CXXFLAGS += -fsanitize=undefined
CXXFLAGS += -fsanitize=shift
CXXFLAGS += -fsanitize=integer-divide-by-zero
CXXFLAGS += -fsanitize=unreachable
CXXFLAGS += -fsanitize=vla-bound
CXXFLAGS += -fsanitize=null
CXXFLAGS += -fsanitize=return
CXXFLAGS += -fsanitize=signed-integer-overflow
CXXFLAGS += -fsanitize=bounds
CXXFLAGS += -fsanitize=alignment
CXXFLAGS += -fsanitize=object-size
CXXFLAGS += -fsanitize=float-divide-by-zero
CXXFLAGS += -fsanitize=float-cast-overflow
CXXFLAGS += -fsanitize=nonnull-attribute
CXXFLAGS += -fsanitize=returns-nonnull-attribute
CXXFLAGS += -fsanitize=bool
CXXFLAGS += -fsanitize=enum
CXXFLAGS += -fsanitize=vptr

CXXFLAGS += -rdynamic								# Allows meaningful backrtaces 
CXXFLAGS += --coverage								# Generate program flow data and inject profiling code

CXX = /opt/gcc-5.3.0/bin/g++
LD = $(CXX) 

LIBS += pthread rt gcc_eh

include ../../ultimate-makefile/Makefile.ultimate
