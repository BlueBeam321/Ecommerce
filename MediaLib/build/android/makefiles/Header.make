
CPP_FLAGS +=-I$(INCLUDE_DIR)
CPP_FLAGS += -D_ISOC99_SOURCE -D__STDC_CONSTANT_MACROS -D__STDC_LIMIT_MACROS -D_C99_SNPRINTF_EXTENSION -D__ANDROID__

CC = $(CROSS_COMPILE)-gcc
CPP = $(CROSS_COMPILE)-g++
AR = $(CROSS_COMPILE)-ar rc
RANLIB = $(CROSS_COMPILE)-ranlib
STRIP = $(CROSS_COMPILE)-strip

SOURCES = $(wildcard *.c) $(wildcard */*.c) $(wildcard */*/*.c)
SOURCES_CPP = $(wildcard *.cpp) $(wildcard */*.cpp) $(wildcard */*/*.cpp)

HEADERS = $(wildcard *.h)
OBJFILES = $(SOURCES:%.c=%.o)
OBJFILES_CPP = $(SOURCES_CPP:%.cpp=%.o)

