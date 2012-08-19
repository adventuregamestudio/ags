LOCAL_PATH := $(call my-dir)/../../../Engine

ADDITIONAL_LIBRARY_PATH := $(call my-dir)/../../nativelibs/$(TARGET_ARCH_ABI)
AGS_COMMON_PATH := $(call my-dir)/../../../Common

include $(CLEAR_VARS)

BASE = ali3dogl.cpp acaudio.cpp acchars.cpp acdebug.cpp acdialog.cpp acfonts.cpp acgfx.cpp acgui.cpp acplatfm.cpp acsound.cpp bigend.cpp misc.cpp routefnd.cpp scrptrt.cpp ac.cpp alogg.c almp3.c ali3dsw.cpp
COMMON = ../Common/csrun.cpp ../Common/Clib32.cpp ../Common/mousew32.cpp ../Common/sprcache.cpp ../Common/cscommon.cpp ../Common/compress.cpp ../Common/lzw.cpp

ALFONT = libsrc/alfont-2.0.9/alfont.c

ALMP3 = libsrc/almp3-2.0.5/decoder/common.c libsrc/almp3-2.0.5/decoder/dct64_i386.c libsrc/almp3-2.0.5/decoder/decode_i386.c libsrc/almp3-2.0.5/decoder/interface.c libsrc/almp3-2.0.5/decoder/layer2.c libsrc/almp3-2.0.5/decoder/layer3.c libsrc/almp3-2.0.5/decoder/tabinit.c

APEG = libsrc/apeg-1.2.1/display.c libsrc/apeg-1.2.1/getbits.c libsrc/apeg-1.2.1/getblk.c libsrc/apeg-1.2.1/gethdr.c libsrc/apeg-1.2.1/getpic.c libsrc/apeg-1.2.1/idct.c libsrc/apeg-1.2.1/motion.c libsrc/apeg-1.2.1/mpeg1dec.c libsrc/apeg-1.2.1/ogg.c libsrc/apeg-1.2.1/recon.c libsrc/apeg-1.2.1/audio/apegcommon.c libsrc/apeg-1.2.1/audio/audio.c libsrc/apeg-1.2.1/audio/dct64.c libsrc/apeg-1.2.1/audio/decode_1to1.c libsrc/apeg-1.2.1/audio/decode_2to1.c libsrc/apeg-1.2.1/audio/decode_4to1.c libsrc/apeg-1.2.1/audio/layer1.c libsrc/apeg-1.2.1/audio/layer2.c libsrc/apeg-1.2.1/audio/layer3.c libsrc/apeg-1.2.1/audio/mpg123.c libsrc/apeg-1.2.1/audio/readers.c libsrc/apeg-1.2.1/audio/tabinit.c libsrc/apeg-1.2.1/audio/vbrhead.c

AASTR = libsrc/aastr-0.1.1/AAROT.c libsrc/aastr-0.1.1/aastr.c libsrc/aastr-0.1.1/aautil.c

ANDROID_SPECIFIC = acpland.cpp ../Android/library/jni/libc.c

LOCAL_MODULE    := agsengine
LOCAL_SRC_FILES := $(ALFONT) $(ALMP3) $(APEG) $(AASTR) $(BASE) $(COMMON) $(ANDROID_SPECIFIC)
LOCAL_CFLAGS    := -g -ffast-math -fsigned-char -Wall -Wfatal-errors -Wno-deprecated-declarations -DALLEGRO_STATICLINK -DTHIS_IS_THE_ENGINE -DLINUX_VERSION -DANDROID_VERSION -DDISABLE_MPEG_AUDIO -DUSE_TREMOR -I$(ADDITIONAL_LIBRARY_PATH)/include -I$(ADDITIONAL_LIBRARY_PATH)/include/freetype2 -I$(AGS_COMMON_PATH) -I$(AGS_COMMON_PATH)/libinclude
LOCAL_CXXFLAGS  := $(LOCAL_CFLAGS) -Wno-write-strings
LOCAL_LDLIBS    := -Wl,-Bstatic -lalleg -lfreetype -lvorbisidec -ltheora -logg -laldmb -ldumb -lstdc++ -Wl,-Bdynamic -lc -ldl -lm -lz -llog -lGLESv1_CM
LOCAL_LDFLAGS   := -Wl,-L$(ADDITIONAL_LIBRARY_PATH)/lib,--allow-multiple-definition
LOCAL_ARM_MODE  := arm

include $(BUILD_SHARED_LIBRARY)


# PE file format helper
include $(CLEAR_VARS)

LOCAL_MODULE    := pe
LOCAL_SRC_FILES := ../Android/library/jni/pe_jni.c ../PSP/launcher/pe.c
LOCAL_CFLAGS    := -O2 -g -ffast-math -fsigned-char -Wall -Wfatal-errors
LOCAL_CXXFLAGS  := $(LOCAL_CFLAGS) -Wno-write-strings
LOCAL_LDLIBS    := -lc -llog
LOCAL_LDFLAGS   :=

include $(BUILD_SHARED_LIBRARY)


# Snowrain plugin
include $(CLEAR_VARS)

LOCAL_MODULE    := ags_snowrain
LOCAL_SRC_FILES := ../Plugins/ags_snowrain/ags_snowrain.cpp
LOCAL_CFLAGS    := -O2 -g -ffast-math -fsigned-char -Wall -Wfatal-errors -DLINUX_VERSION -DANDROID_VERSION
LOCAL_CXXFLAGS  := $(LOCAL_CFLAGS) -Wno-write-strings
LOCAL_LDLIBS    := -Wl,-Bstatic -lstdc++ -Wl,-Bdynamic -lc -lm -llog
LOCAL_LDFLAGS   :=

include $(BUILD_SHARED_LIBRARY)


# AGSBlend plugin
include $(CLEAR_VARS)

LOCAL_MODULE    := agsblend
LOCAL_SRC_FILES := ../Plugins/agsblend/AGSBlend.cpp
LOCAL_CFLAGS    := -O2 -g -ffast-math -fsigned-char -Wall -Wfatal-errors -DLINUX_VERSION -DANDROID_VERSION
LOCAL_CXXFLAGS  := $(LOCAL_CFLAGS) -Wno-write-strings
LOCAL_LDLIBS    := -Wl,-Bstatic -lstdc++ -Wl,-Bdynamic -lc -lm -llog
LOCAL_LDFLAGS   :=

include $(BUILD_SHARED_LIBRARY)


# AGSflashlight plugin
include $(CLEAR_VARS)

LOCAL_MODULE    := agsflashlight
LOCAL_SRC_FILES := ../Plugins/AGSflashlight/agsflashlight.cpp
LOCAL_CFLAGS    := -O2 -g -ffast-math -fsigned-char -Wall -Wfatal-errors -DLINUX_VERSION -DANDROID_VERSION
LOCAL_CXXFLAGS  := $(LOCAL_CFLAGS) -Wno-write-strings
LOCAL_LDLIBS    := -Wl,-Bstatic -lstdc++ -Wl,-Bdynamic -lc -lm -llog
LOCAL_LDFLAGS   :=

include $(BUILD_SHARED_LIBRARY)


# AGSlua plugin
include $(CLEAR_VARS)

LOCAL_MODULE    := agslua
LOCAL_SRC_FILES := ../Plugins/agslua/agslua/agslua/agslua_autogen.cpp \
../Plugins/agslua/agslua/agslua/AGSLua_Main.cpp \
../Plugins/agslua/agslua/agslua/AGSManagedObjects.cpp \
../Plugins/agslua/agslua/agslua/CompressedLuaChunks.cpp \
../Plugins/agslua/agslua/agslua/Internal.cpp \
../Plugins/agslua/agslua/agslua/LuaCustomisation.cpp \
../Plugins/agslua/agslua/agslua/LuaFromAGS.cpp \
../Plugins/agslua/agslua/agslua/LuaValueList.cpp \
../Plugins/agslua/agslua/agslua/SerializeLuaUniverse.cpp \
../Plugins/agslua/agslua/agslua/AGSStructFields.cpp \
../Plugins/agslua/agslua/agslua/DLLStuff.cpp \
../Plugins/agslua/agslua/agslua/pluto.c \
../Plugins/agslua/agslua/agslua/pdep.c \
../Plugins/agslua/agslua/agslua/lzio.c
LOCAL_CFLAGS    := -O2 -g -ffast-math -fsigned-char -Wall -Wfatal-errors -DTHIS_IS_THE_PLUGIN -DLINUX_VERSION -DANDROID_VERSION -I$(AGS_COMMON_PATH)/../Plugins/agslua/agslua/lualibhelp/include -I$(ADDITIONAL_LIBRARY_PATH)/include
LOCAL_CXXFLAGS  := $(LOCAL_CFLAGS) -Wno-write-strings
LOCAL_LDLIBS    := -Wl,-Bstatic -lstdc++ -llua -Wl,-Bdynamic -lc -lm -llog -lz
LOCAL_LDFLAGS   := -Wl,-L$(ADDITIONAL_LIBRARY_PATH)/lib

include $(BUILD_SHARED_LIBRARY)
