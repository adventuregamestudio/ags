UNAME := $(shell uname)

ifeq ($(UNAME), Darwin)
include Makefile-defs.osx
else
include Makefile-defs.linux
endif

include Makefile-objs
OBJS_C_CPP = $(ALFONT) $(ALMP3) $(ALOGG) $(APEG) $(AASTR) $(HQ2X3X) $(CDA) $(PLUGINS) $(BASE) $(BASE_PLATFORM)
OBJS_C = $(OBJS_C_CPP:.cpp=.o)
OBJS = $(OBJS_C:.c=.o)

OBJS_COMMON_CPP = $(COMMON)
OBJS_COMMON = $(OBJS_COMMON_CPP:.cpp=.o)
DEPFILES = $(OBJS:.o=.d) $(OBJS_COMMON:.o=.d)

-include config.mak

.PHONY: printflags clean install uninstall rebuild

all: printflags ags

ags: $(OBJS) common.a 
	@echo "Linking engine..."
	@$(CC) -o $@ $^ $(CFLAGS) $(LDFLAGS) $(LIBS)

common.a: $(OBJS_COMMON)
	@echo "Linking common library..."
	@$(AR) rcs $@ $^

-include $(DEPFILES)

%.o: %.c
	@echo $@
	@$(CC) $(CFLAGS) -MD -c -o $@ $<

%.o: %.cpp
	@echo $@
	@$(CXX) $(CXXFLAGS) -MD -c -o $@ $<

printflags:
	@echo "CFLAGS =" $(CFLAGS) "\n"
	@echo "CXXFLAGS =" $(CXXFLAGS) "\n"
	@echo "LDFLAGS =" $(LDFLAGS) "\n"
	@echo "LIBS =" $(LIBS) "\n"

rebuild: clean all

clean:
	@echo "Cleaning..."
	@rm -f ags agsgame.log common.a $(OBJS) $(OBJS_COMMON) $(DEPFILES)

install: ags
	mkdir -p $(PREFIX)/bin
	cp -t $(PREFIX)/bin ags

uninstall:
	rm -f $(PREFIX)/bin/ags
