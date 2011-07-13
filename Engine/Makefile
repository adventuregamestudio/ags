# Build the engine as an EBOOT that can only run ac2game.dat in the current directory
STANDALONE = 1

# Enable gprof profiling in the engine, requires a rebuild of all files
PROFILING = 0

# The name of the engine prx must be known to the launcher and the engine too
TARGET = ags321

PSP_SPECIFIC = acplpsp.o ../PSP/kernel/kernel.o ../PSP/exception/utility/exception.o

BASE = acaudio.o acchars.o acdebug.o acdialog.o acfonts.o acgfx.o acgui.o acplatfm.o acsound.o bigend.o misc.o routefnd.o scrptrt.o ac.o alogg.o almp3.o ali3dsw.o
COMMON = ../Common/csrun.o ../Common/Clib32.o ../Common/mousew32.o ../Common/sprcache.o ../Common/cscommon.o ../Common/compress.o ../Common/lzw.o
LIBSRC = libsrc/hq2x/hq2x3x.o

ALFONT = libsrc/alfont-2.0.9/alfont.o

ALMP3 = libsrc/almp3-2.0.5/decoder/common.o libsrc/almp3-2.0.5/decoder/dct64_i386.o libsrc/almp3-2.0.5/decoder/decode_i386.o libsrc/almp3-2.0.5/decoder/interface.o libsrc/almp3-2.0.5/decoder/layer2.o libsrc/almp3-2.0.5/decoder/layer3.o libsrc/almp3-2.0.5/decoder/tabinit.o

APEG = libsrc/apeg-1.2.1/display.o libsrc/apeg-1.2.1/getbits.o libsrc/apeg-1.2.1/getblk.o libsrc/apeg-1.2.1/gethdr.o libsrc/apeg-1.2.1/getpic.o libsrc/apeg-1.2.1/idct.o libsrc/apeg-1.2.1/motion.o libsrc/apeg-1.2.1/mpeg1dec.o libsrc/apeg-1.2.1/ogg.o libsrc/apeg-1.2.1/recon.o libsrc/apeg-1.2.1/audio/apegcommon.o libsrc/apeg-1.2.1/audio/audio.o libsrc/apeg-1.2.1/audio/dct64.o libsrc/apeg-1.2.1/audio/decode_1to1.o libsrc/apeg-1.2.1/audio/decode_2to1.o libsrc/apeg-1.2.1/audio/decode_4to1.o libsrc/apeg-1.2.1/audio/layer1.o libsrc/apeg-1.2.1/audio/layer2.o libsrc/apeg-1.2.1/audio/layer3.o libsrc/apeg-1.2.1/audio/mpg123.o libsrc/apeg-1.2.1/audio/readers.o libsrc/apeg-1.2.1/audio/tabinit.o libsrc/apeg-1.2.1/audio/vbrhead.o

AASTR = libsrc/aastr-0.1.1/AAROT.o libsrc/aastr-0.1.1/aastr.o libsrc/aastr-0.1.1/aautil.o

OBJS = $(LIBSRC) $(ALFONT) $(ALMP3) $(APEG) $(AASTR) $(BASE) $(COMMON) $(PSP_SPECIFIC)

INCDIR = ../Common ../Common/libinclude $(shell psp-config --pspdev-path)/psp/include/freetype2
LIBDIR = ../PSP/lib
CFLAGS = -O2 -g -G0 -msingle-float -ffast-math -Wfatal-errors -DALLEGRO_STATICLINK -DTHIS_IS_THE_ENGINE -DLINUX_VERSION -DPSP_VERSION -DDISABLE_MPEG_AUDIO
CXXFLAGS = $(CFLAGS) -fno-rtti -Wno-write-strings
ASFLAGS = $(CFLAGS)
LIBS = -lalleg -lalleg-main -lfreetype -lz -lvorbisidec -ltheoradec -logg -laldmb -ldumb -lc -lstdc++ -lm
LIBS += -lpsppower -lpspdebug -lpspgu -lpspctrl -lpspge -lpspdisplay -lpsprtc -lpspaudio -lpspsystemctrl_user -lpspmath
LDFLAGS =

ifeq ($(PROFILING), 1)
CFLAGS += -pg -DPSP_ENABLE_PROFILING
CXXFLAGS += -pg -DPSP_ENABLE_PROFILING
LIBS += -lpspprof
else
CXXFLAGS += -fomit-frame-pointer
endif

ifeq ($(STANDALONE), 1)
EXTRA_TARGETS = EBOOT.PBP
PSP_EBOOT_TITLE = Adventure Game Studio 3.21
PSP_EBOOT_ICON = ../PSP/media/ICON0.PNG
PSP_EBOOT_UNKPNG = ../PSP/media/PIC0.PNG
PSP_EBOOT_PIC1 = ../PSP/media/PIC1.PNG
PSP_EBOOT_SND0 = NULL
endif

PSP_LARGE_MEMORY = 1
PSP_FW_VERSION = 371
BUILD_PRX = 1
ENCRYPT = 0

PSPSDK=$(shell psp-config --pspsdk-path)
include $(PSPSDK)/lib/build.mak
