UNAME := $(shell uname)

INCDIR = agslua/lualibhelp/include/ ../../Engine ../../Common
LIBDIR =
CC = gcc
CXX = g++
CFLAGS = -fPIC -fvisibility=hidden -O2 -g -Wall $(shell pkg-config --cflags lua5.1-c++)
LIBS = -lm -lstdc++ -lz $(shell pkg-config --libs)

ifeq ($(UNAME), Darwin)
TARGET = libagslua.dylib
CFLAGS += -DMAC_VERSION
else
TARGET = libagslua.so
CFLAGS += -DLINUX_VERSION
endif

CXXFLAGS = $(CFLAGS)

include Makefile-objs


CFLAGS   := $(addprefix -I,$(INCDIR)) $(CFLAGS)
CXXFLAGS := $(CFLAGS) $(CXXFLAGS)
ASFLAGS  := $(CFLAGS) $(ASFLAGS)


.PHONY: clean

all: $(TARGET)

$(TARGET): $(OBJS)
	@$(CC) -shared -dynamiclib -o $@ $^ $(CFLAGS) $(LDFLAGS) $(LIBS)

%.o: %.c
	@echo $@
	@$(CC) $(CFLAGS) -c -o $@ $<

%.o: %.cpp
	@echo $@
	@$(CXX) $(CXXFLAGS) -c -o $@ $<

clean:
	@rm -f $(TARGET) $(OBJS)
