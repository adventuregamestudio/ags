TARGET = agslauncher

OBJS = main.o ../kernel/kernel.o

CFLAGS = -O2 -g -G0 -msingle-float -ffast-math -Wfatal-errors
CXXFLAGS = $(CFLAGS) -fno-rtti -Wno-write-strings -fomit-frame-pointer
ASFLAGS = $(CFLAGS)
LIBS =
LDFLAGS =

EXTRA_TARGETS = EBOOT.PBP
PSP_EBOOT_TITLE = Adventure Game Studio 3.21 Launcher

PSP_EBOOT_ICON = ../media/ICON0_White.PNG
PSP_EBOOT_UNKPNG = ../media/PIC0.PNG
PSP_EBOOT_PIC1 = ../media/PIC1.PNG
PSP_EBOOT_SND0 = NULL

PSP_LARGE_MEMORY = 1

PSP_FW_VERSION = 371
BUILD_PRX = 1
ENCRYPT = 0

PSPSDK=$(shell psp-config --pspsdk-path)
include $(PSPSDK)/lib/build.mak
