TARGET = exception_handler_test
OBJS = ../utility/exception.o main.o

#To build for custom firmware:
BUILD_PRX = 1

#CFLAGS = -O3 -G0 -Wall
CFLAGS = -O3 -frename-registers -G0 -Wall
CXXFLAGS = $(CFLAGS) -fno-exceptions -fno-rtti
ASFLAGS = $(CFLAGS)
LIBDIR =

LIBS = 
LDFLAGS =
EXTRA_TARGETS = EBOOT.PBP
PSP_EBOOT_TITLE = Exception Handler Test
#PSP_EBOOT_ICON = ICON0.PNG
PSPSDK=$(shell psp-config --pspsdk-path)
include $(PSPSDK)/lib/build.mak
