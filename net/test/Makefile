OUTPUT = net-test

ROOT = ../..
PET = $(ROOT)/pet

INCLUDE_DIRS += $(ROOT)
INCLUDE_DIRS += $(ROOT)/net
INCLUDE_DIRS += $(ROOT)/scheduler

SOURCES += main.cpp

SOURCES += $(ROOT)/scheduler/ports/gcc-linux-um/Profile.cpp

INCLUDE_DIRS += .
INCLUDE_DIRS += $(PET)

CXXFLAGS += -std=c++17 
CXXFLAGS += -g3 
#CXXFLAGS += -Og
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
CXXFLAGS += -Wconversion
CXXFLAGS += -Wuseless-cast
CXXFLAGS += -Wcast-align
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
CXXFLAGS += -fsanitize=undefined

CXXFLAGS += -rdynamic								# Allows meaningful backtraces 
CXXFLAGS += --coverage								# Generate program flow data and inject profiling code

LD = $(CXX) 

include $(ROOT)/test/toolchains/Compiler.host.mk
include $(ROOT)/test/ultimate-makefile/Makefile.ultimate

enter:
	CD_TO=$(ROOT) make -C $(ROOT)/test/toolchains/ enter-the-matrix 
