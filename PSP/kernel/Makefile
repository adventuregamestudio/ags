TARGET = kernel
OBJS = main.o

# Use the kernel's small inbuilt libc
USE_KERNEL_LIBC = 1
# Use only kernel libraries
USE_KERNEL_LIBS = 1

INCDIR = 
CFLAGS = -O2 -G0 -Wall
CXXFLAGS = $(CFLAGS) -fno-exceptions -fno-rtti
ASFLAGS = $(CFLAGS)
LDFLAGS = -mno-crt0 -nostartfiles

BUILD_PRX = 1
PRX_EXPORTS = exports.exp

LIBDIR =

LIBS =

PSPSDK=$(shell psp-config --pspsdk-path)
include $(PSPSDK)/lib/build.mak
