#-----------------------------------------------------------------------------#
#
# CMake setup
#

cmake_minimum_required(VERSION 3.13..3.14)

#-----------------------------------------------------------------------------#
#
# Build options
#

# Set the project name.
project(allegro
    LANGUAGES C CXX
    VERSION 4.4.3
)

if(WIN32)
    enable_language(RC OPTIONAL)
endif(WIN32)

# Search in the `cmake' directory for additional CMake modules.
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")


#-----------------------------------------------------------------------------#
#
# Build options
#

option(MAGIC_MAIN "Enable magic main (Unix)" off)


#-----------------------------------------------------------------------------#
#
# Unix platform checks
#

include(CheckIncludeFiles)
include(CheckFunctionExists)
include(CheckCSourceCompiles)
include(CheckCSourceRuns)
include(FindPkgConfig)
include(TestBigEndian)

if(UNIX)
    test_big_endian(ALLEGRO_BIG_ENDIAN)
    if(NOT ALLEGRO_BIG_ENDIAN)
        set(ALLEGRO_LITTLE_ENDIAN 1)
    endif(NOT ALLEGRO_BIG_ENDIAN)

    check_include_files(dirent.h ALLEGRO_HAVE_DIRENT_H)
    check_include_files(inttypes.h ALLEGRO_HAVE_INTTYPES_H)

    check_include_files(linux/awe_voice.h ALLEGRO_HAVE_LINUX_AWE_VOICE_H)
    check_include_files(linux/input.h ALLEGRO_HAVE_LINUX_INPUT_H)
    # On some systems including linux/joystick.h without sys/types.h results
    # in conflicting definitions of fd_set.
    check_include_files("sys/types.h;linux/joystick.h" ALLEGRO_HAVE_LINUX_JOYSTICK_H)
    check_include_files(linux/soundcard.h ALLEGRO_HAVE_LINUX_SOUNDCARD_H)
    check_include_files(machine/soundcard.h ALLEGRO_HAVE_MACHINE_SOUNDCARD_H)
    check_include_files(soundcard.h ALLEGRO_HAVE_SOUNDCARD_H)
    check_include_files(stdint.h ALLEGRO_HAVE_STDINT_H)
    check_include_files(sys/io.h ALLEGRO_HAVE_SYS_IO_H)
    check_include_files(sys/stat.h ALLEGRO_HAVE_SYS_STAT_H)
    check_include_files(sys/time.h ALLEGRO_HAVE_SYS_TIME_H)
    check_include_files(sys/soundcard.h ALLEGRO_HAVE_SYS_SOUNDCARD_H)
    check_include_files(sys/utsname.h ALLEGRO_HAVE_SYS_UTSNAME_H)

    check_function_exists(getexecname ALLEGRO_HAVE_GETEXECNAME)
    check_function_exists(memcmp ALLEGRO_HAVE_MEMCMP)
    check_function_exists(mkstemp ALLEGRO_HAVE_MKSTEMP)
    check_function_exists(mmap ALLEGRO_HAVE_MMAP)
    check_function_exists(mprotect ALLEGRO_HAVE_MPROTECT)
    check_function_exists(sched_yield ALLEGRO_HAVE_SCHED_YIELD)
    check_function_exists(stricmp ALLEGRO_HAVE_STRICMP)
    check_function_exists(strlwr ALLEGRO_HAVE_STRLWR)
    check_function_exists(strupr ALLEGRO_HAVE_STRUPR)
    check_function_exists(sysconf ALLEGRO_HAVE_SYSCONF)

    check_c_source_compiles("
        #include <sys/procfs.h>
        #include <sys/ioctl.h>
        int main(void) {
            struct prpsinfo psinfo;
            ioctl(0, PIOCPSINFO, &psinfo);
            return 0;
        }"
        ALLEGRO_HAVE_SV_PROCFS_H
        )
    check_c_source_compiles("
        #include <sys/procfs.h>
        int main(void) {
            struct prpsinfo psinfo;
            psinfo.pr_argc = 0;
            return 0;
        }"
        ALLEGRO_HAVE_PROCFS_ARGCV
        )

    check_c_source_compiles("
        #include <unistd.h>
        #include <sys/mman.h>
        int main(void) {
            void *x = MAP_FAILED;
        }"
        MAP_FAILED_DEFINED)
    if(NOT MAP_FAILED_DEFINED)
        set(MAP_FAILED "((void *) -1)")
    endif()

    check_c_source_runs("
        static int notsupported = 1;
        void test_ctor (void) __attribute__((constructor));
        void test_ctor (void) { notsupported = 0; }
        int main(void) { return (notsupported); }
        "
        ALLEGRO_USE_CONSTRUCTOR)

    find_library(RT_LIBRARY rt)
    mark_as_advanced(RT_LIBRARY)
    check_c_source_compiles("
        #include <time.h>
        int main(void) {
            struct timespec new_time_ns;
            clock_gettime(CLOCK_MONOTONIC, &new_time_ns);
            return 0;
        }"
        ALLEGRO_HAVE_POSIX_MONOTONIC_CLOCK
        )

    if(MAGIC_MAIN)
        set(ALLEGRO_WITH_MAGIC_MAIN 1)
    endif(MAGIC_MAIN)

    # XXX const
    # XXX inline
    # XXX size_t
endif(UNIX)


#-----------------------------------------------------------------------------#
#
# Main library
#

add_library(allegro)

target_compile_definitions(allegro PRIVATE ALLEGRO_SRC)

target_compile_definitions (allegro
    INTERFACE 
        ALLEGRO_STATICLINK
        ALLEGRO_NO_COMPATIBILITY 
        ALLEGRO_NO_FIX_ALIASES 
        ALLEGRO_NO_FIX_CLASS
)

# ALLEGRO_SRC_FILES
target_sources(allegro PRIVATE
    src/allegro.c
    src/blit.c
    src/bmp.c
    src/clip3d.c
    src/clip3df.c
    src/colblend.c
    src/color.c
    src/config.c
    src/datafile.c
    src/dataregi.c
    src/digmid.c
    src/dither.c
    src/dispsw.c
    src/drvlist.c
    src/file.c
    src/fli.c
    src/flood.c
    src/font.c
    src/fontbios.c
    src/fontbmp.c
    src/fontdat.c
    src/fontgrx.c
    src/fonttxt.c
    src/fsel.c
    src/gfx.c
    src/glyph.c
    src/graphics.c
    src/gsprite.c
    src/gui.c
    src/guiproc.c
    src/inline.c
    src/joystick.c
    src/keyboard.c
    src/lbm.c
    src/libc.c
    src/lzss.c
    src/math.c
    src/math3d.c
    src/midi.c
    src/mixer.c
    src/modesel.c
    src/mouse.c
    src/pcx.c
    src/poly3d.c
    src/polygon.c
    src/quantize.c
    src/quat.c
    src/readbmp.c
    src/readfont.c
    src/readsmp.c
    src/rle.c
    src/rotate.c
    src/rsfb.c
    src/scene3d.c
    src/sound.c
    src/spline.c
    src/stream.c
    src/text.c
    src/tga.c
    src/timer.c
    src/unicode.c
    src/vtable.c
    src/vtable15.c
    src/vtable16.c
    src/vtable24.c
    src/vtable32.c
    src/vtable8.c
)

# ALLEGRO_SRC_C_FILES
target_sources(allegro PRIVATE
    src/c/cblit16.c
    src/c/cblit24.c
    src/c/cblit32.c
    src/c/cblit8.c
    src/c/ccpu.c
    src/c/ccsprite.c
    src/c/cgfx15.c
    src/c/cgfx16.c
    src/c/cgfx24.c
    src/c/cgfx32.c
    src/c/cgfx8.c
    src/c/cmisc.c
    src/c/cscan15.c
    src/c/cscan16.c
    src/c/cscan24.c
    src/c/cscan32.c
    src/c/cscan8.c
    src/c/cspr15.c
    src/c/cspr16.c
    src/c/cspr24.c
    src/c/cspr32.c
    src/c/cspr8.c
    src/c/cstretch.c
    src/c/czscan15.c
    src/c/czscan16.c
    src/c/czscan24.c
    src/c/czscan32.c
    src/c/czscan8.c
    src/misc/ccolconv.c
    src/misc/colconv.c
)

# set(ALLEGRO_INCLUDE_ALLEGRO_FILES
target_sources(allegro PRIVATE
    include/allegro/3d.h
    include/allegro/3dmaths.h
    include/allegro/alcompat.h
    include/allegro/alinline.h
    include/allegro/base.h
    include/allegro/color.h
    include/allegro/compiled.h
    include/allegro/config.h
    include/allegro/datafile.h
    include/allegro/debug.h
    include/allegro/digi.h
    include/allegro/draw.h
    include/allegro/file.h
    include/allegro/fix.h
    include/allegro/fixed.h
    include/allegro/fli.h
    include/allegro/fmaths.h
    include/allegro/font.h
    include/allegro/gfx.h
    include/allegro/graphics.h
    include/allegro/gui.h
    include/allegro/joystick.h
    include/allegro/keyboard.h
    include/allegro/lzss.h
    include/allegro/matrix.h
    include/allegro/midi.h
    include/allegro/mouse.h
    include/allegro/palette.h
    include/allegro/quat.h
    include/allegro/rle.h
    include/allegro/sound.h
    include/allegro/stream.h
    include/allegro/system.h
    include/allegro/text.h
    include/allegro/timer.h
    include/allegro/unicode.h
)

# set(ALLEGRO_INCLUDE_ALLEGRO_INLINE_FILES
target_sources(allegro PRIVATE
    include/allegro/inline/3dmaths.inl
    include/allegro/inline/asm.inl
    include/allegro/inline/color.inl
    include/allegro/inline/draw.inl
    include/allegro/inline/fix.inl
    include/allegro/inline/fmaths.inl
    include/allegro/inline/gfx.inl
    include/allegro/inline/matrix.inl
    include/allegro/inline/rle.inl
    include/allegro/inline/system.inl
)

# set(ALLEGRO_INCLUDE_ALLEGRO_INTERNAL_FILES
target_sources(allegro PRIVATE
    include/allegro/internal/aintern.h
    include/allegro/internal/aintvga.h
    include/allegro/internal/alconfig.h
)

# set(ALLEGRO_INCLUDE_ALLEGRO_PLATFORM_FILES
target_sources(allegro PRIVATE
    include/allegro/platform/aintbeos.h
    include/allegro/platform/aintdos.h
    include/allegro/platform/aintlnx.h
    include/allegro/platform/aintmac.h
    include/allegro/platform/aintosx.h
    include/allegro/platform/aintpsp.h
    include/allegro/platform/aintios.h
    include/allegro/platform/aintand.h
    include/allegro/platform/aintqnx.h
    include/allegro/platform/aintunix.h
    include/allegro/platform/aintwin.h
    include/allegro/platform/al386gcc.h
    include/allegro/platform/al386vc.h
    include/allegro/platform/al386wat.h
    include/allegro/platform/albcc32.h
    include/allegro/platform/albecfg.h
    include/allegro/platform/albeos.h
    include/allegro/platform/aldjgpp.h
    include/allegro/platform/aldmc.h
    include/allegro/platform/aldos.h
    include/allegro/platform/almac.h
    include/allegro/platform/almaccfg.h
    include/allegro/platform/almngw32.h
    include/allegro/platform/almsvc.h
    include/allegro/platform/alosx.h
    include/allegro/platform/alosxcfg.h
    # include/allegro/platform/alplatf.h.cmake
    include/allegro/platform/alpsp.h
    include/allegro/platform/alpspcfg.h
    include/allegro/platform/alios.h
    include/allegro/platform/alioscfg.h
    include/allegro/platform/aland.h
    include/allegro/platform/alandcfg.h
    include/allegro/platform/alqnx.h
    include/allegro/platform/alqnxcfg.h
    include/allegro/platform/alucfg.h
    include/allegro/platform/alunix.h
    # include/allegro/platform/alunixac.h.cmake
    # include/allegro/platform/alunixac.hin
    include/allegro/platform/alwatcom.h
    include/allegro/platform/alwin.h
    include/allegro/platform/astdint.h
    include/allegro/platform/macdef.h
)


#-----------------------------------------------------------------------------#
#
# Compiler and platform setup
#

if(CMAKE_COMPILER_IS_GNUCC)
    set(COMPILER_GCC 1)
    set(ALLEGRO_GCC 1)
endif(CMAKE_COMPILER_IS_GNUCC)

if(MSVC)
    set(COMPILER_MSVC 1)
    set(ALLEGRO_MSVC 1)
endif(MSVC)

if(WIN32)
    set(ALLEGRO_WINDOWS 1)
endif()

if(APPLE)
    set(ALLEGRO_MACOSX 1)
    set(ALLEGRO_DARWIN 1)
endif(APPLE)

if(UNIX AND NOT APPLE)
    set(ALLEGRO_UNIX 1)
endif()

if(IOS)
    set(ALLEGRO_IOS 1)

    set(ALLEGRO_IOS_ARCH arm64 CACHE STRING "iOS Architecture")
    set_property(CACHE ALLEGRO_IOS_ARCH PROPERTY STRINGS arm64 armv7 armv7s i386 x86_64)

    set(ALLEGRO_IOS_SDK iphoneos CACHE STRING "iOS SDK")
    set_property(CACHE ALLEGRO_IOS_SDK PROPERTY STRINGS iphoneos iphonesimulator)

    execute_process(COMMAND xcrun --sdk ${ALLEGRO_IOS_SDK} --show-sdk-path
        OUTPUT_VARIABLE SDK_PATH
        OUTPUT_STRIP_TRAILING_WHITESPACE)
    set(IOS_ADDITIONAL_LIBRARY_PATH "$(pwd)/../../../nativelibs/${ALLEGRO_IOS_ARCH}")
    set(IOS_PLATFORM_INCLUDE "${SDK_PATH}/usr/include")
    set(IOS_PLATFORM_LIB "${SDK_PATH}/usr/lib")

    target_compile_options(allegro PRIVATE -arch ${ALLEGRO_IOS_ARCH})
endif(IOS)

if(ANDROID)
    set(ALLEGRO_ANDROID 1)
endif(ANDROID)

target_compile_definitions(allegro PUBLIC ALLEGRO_STATICLINK)
target_compile_definitions(allegro PUBLIC $<$<CONFIG:DEBUG>:DEBUGMODE=1>)


#-----------------------------------------------------------------------------#

# Not sure if we want to support disabling these any more.
set(ALLEGRO_COLOR8 1)
set(ALLEGRO_COLOR16 1)
set(ALLEGRO_COLOR24 1)
set(ALLEGRO_COLOR32 1)

set(ALLEGRO_NO_ASM 1)
# ALLEGRO_MMX left undefined
# ALLEGRO_SSE left undefined


#-----------------------------------------------------------------------------#
#
# Platform drivers
#

# -- Unix --

option(WANT_OSS "Build OSS support" off)
option(WANT_ALSA "Build ALSA support" on)
option(WANT_JACK "Build JACK support" on)

if(ALLEGRO_UNIX) # not MACOSX
    target_sources(allegro PRIVATE
        src/unix/alsa9.c
        src/unix/alsamidi.c
        src/unix/sdl2digi.c
        src/unix/arts.c
        src/unix/sgial.c
        src/unix/jack.c
        src/unix/udjgpp.c
        src/unix/udrvlist.c
        src/unix/udummy.c
        src/unix/uesd.c
        src/unix/ufile.c
        src/unix/ugfxdrv.c
        src/unix/ujoydrv.c
        src/unix/ukeybd.c
        src/unix/umain.c
        src/unix/umodules.c
        src/unix/umouse.c
        src/unix/uoss.c
        src/unix/uossmidi.c
        src/unix/uptimer.c
        src/unix/usigalrm.c
        src/unix/usnddrv.c
        src/unix/ustimer.c
        src/unix/usystem.c
        src/unix/uthreads.c
        src/unix/utimer.c
        # src/misc/modexsms.c

        # May be used without enabling the entire Linux console port.
        src/linux/ljoy.c
    )

    find_package(Threads)
    if(NOT CMAKE_USE_PTHREADS_INIT)
        message(FATAL_ERROR "Unix port requires pthreads support.")
    endif()
    set(ALLEGRO_HAVE_LIBPTHREAD 1)
    target_link_libraries(allegro PUBLIC m ${CMAKE_THREAD_LIBS_INIT})

    target_link_libraries(allegro PUBLIC ${CMAKE_DL_LIBS})

    if(ALLEGRO_HAVE_POSIX_MONOTONIC_CLOCK)
        target_link_libraries(allegro PUBLIC ${RT_LIBRARY})
    endif(ALLEGRO_HAVE_POSIX_MONOTONIC_CLOCK)

    if(WANT_OSS)
        include(AllegroFindOSS)
        if(OSS_FOUND)
            set(ALLEGRO_WITH_OSSDIGI 1)
            set(ALLEGRO_WITH_OSSMIDI 1)
            target_include_directories(allegro PRIVATE ${OSS_INCLUDE_DIR})
        endif(OSS_FOUND)
    endif(WANT_OSS)

    if(WANT_ALSA)
        pkg_check_modules(ALSA alsa)
        if(ALSA_FOUND)
            # ALSA 0.5 is beyond obsolete.
            set(ALLEGRO_ALSA_VERSION 9)
            set(ALLEGRO_WITH_ALSADIGI 1)
            set(ALLEGRO_WITH_ALSAMIDI 1)
            target_include_directories(allegro PRIVATE ${ALSA_INCLUDE_DIRS})
            target_link_libraries(allegro PUBLIC ${ALSA_LIBRARIES})
        endif(ALSA_FOUND)
    endif(WANT_ALSA)

    pkg_check_modules(SDL2 sdl2)
    if(SDL2_FOUND)
        set(ALLEGRO_WITH_SDL2DIGI 1)
        target_include_directories(allegro PRIVATE ${SDL2_INCLUDE_DIRS})
        target_link_directories(allegro PUBLIC ${SDL2_LIBRARY_DIRS})
        target_link_libraries(allegro PUBLIC ${SDL2_LIBRARIES})
    endif(SDL2_FOUND)

    if(WANT_JACK)
        pkg_check_modules(JACK jack)
        if(JACK_FOUND)
            set(ALLEGRO_WITH_JACKDIGI 1)
            target_include_directories(allegro PRIVATE ${JACK_INCLUDE_DIRS})
            target_link_libraries(allegro PUBLIC ${JACK_LIBRARIES})
        endif(JACK_FOUND)
    endif(WANT_JACK)

endif(ALLEGRO_UNIX)

# -- X11 --

option(WANT_X11 "Want X11 support (Unix)" on)

if(ALLEGRO_UNIX AND WANT_X11)
    find_package(X11)
    if(X11_FOUND)
        set(ALLEGRO_WITH_XWINDOWS 1)
    endif()
endif()

if(ALLEGRO_WITH_XWINDOWS)
    target_include_directories(allegro PRIVATE ${X11_INCLUDE_DIR})

    target_sources(allegro PRIVATE
        src/x/xgfxdrv.c
        src/x/xkeyboard.c
        src/x/xmouse.c
        src/x/xsystem.c
        src/x/xtimer.c
        src/x/xvtable.c
        src/x/xwin.c
        src/x/xdga2.c
        src/x/xdga2s.s
        src/x/xwins.s
    )

    target_link_libraries(allegro PUBLIC ${X11_LIBRARIES})

    if(X11_XShm_FOUND)
        set(ALLEGRO_XWINDOWS_WITH_SHM 1)
        target_link_libraries(allegro PUBLIC ${X11_Xext_LIB})
    endif()

    if(X11_Xcursor_FOUND)
        set(ALLEGRO_XWINDOWS_WITH_XCURSOR 1)
        target_link_libraries(allegro PUBLIC ${X11_Xcursor_LIB})
    endif()

    if(X11_Xcursor_FOUND)
        set(ALLEGRO_XWINDOWS_WITH_XCURSOR 1)
        target_link_libraries(allegro PUBLIC ${X11_Xcursor_LIB})
    endif()

    if(X11_Xpm_FOUND)
        set(ALLEGRO_XWINDOWS_WITH_XPM 1)
        target_link_libraries(allegro PUBLIC ${X11_Xpm_LIB})
    endif()

    find_library(X11_Xxf86vm_LIB Xxf86vm ${X11_LIB_SEARCH_PATH})
    mark_as_advanced(X11_Xxf86vm_LIB)
    if(X11_xf86vmode_FOUND)
        set(ALLEGRO_XWINDOWS_WITH_XF86VIDMODE 1)
        target_link_libraries(allegro PUBLIC ${X11_Xxf86vm_LIB})
    endif()

    check_library_exists(X11 XOpenIM "${X11_LIB_SEARCH_PATH}" XIM_FOUND)
    if(XIM_FOUND)
        set(ALLEGRO_XWINDOWS_WITH_XIM 1)
    endif(XIM_FOUND)

    check_library_exists(Xxf86dga XDGAQueryExtension
        "${X11_LIB_SEARCH_PATH}" XDGA_FOUND)
    if(XDGA_FOUND)
        set(ALLEGRO_XWINDOWS_WITH_XF86DGA2 1)
        target_link_libraries(allegro PUBLIC Xxf86dga ${X11_LIBRARIES})
    endif()
endif(ALLEGRO_WITH_XWINDOWS)

# -- Windows --

if(WIN32)
    target_sources(allegro PRIVATE
        src/win/asmlock.s
        src/win/dllver.rc
        src/win/gdi.c
        src/win/wddaccel.c
        src/win/wddbmp.c
        src/win/wddbmpl.c
        src/win/wddraw.c
        src/win/wddfull.c
        src/win/wddlock.c
        src/win/wddmode.c
        src/win/wddovl.c
        src/win/wddwin.c
        src/win/wdsinput.c
        src/win/wdsndmix.c
        src/win/wdsound.c
        src/win/wsndwo.c
        src/win/wdxver.c
        src/win/wdispsw.c
        src/win/wfile.c
        src/win/wgdi.c
        src/win/wgfxdrv.c
        src/win/winput.c
        src/win/wjoydrv.c
        src/win/wjoydx.c
        src/win/wjoyhelp.c
        src/win/wjoyw32.c
        src/win/wkeybd.c
        src/win/wmidi.c
        src/win/wmouse.c
        src/win/wsnddrv.c
        src/win/wsystem.c
        src/win/wthread.c
        src/win/wtimer.c
        src/win/wwnd.c
    )

    find_package(DDraw)
    find_package(DInput)
    find_package(DSound)
    find_package(DXGuid)

    if(NOT DDRAW_FOUND OR NOT DINPUT_FOUND OR NOT DSOUND_FOUND OR NOT DXGUID_FOUND)
        message(FATAL_ERROR "DirectX required for Windows port. You might need to add DirectX include and lib directories to your INCLUDE and LIB environment variables.")
    endif()

    # Don't use include directories from DirectX SDK as Windows SDK contains them now.
    # target_include_directories(allegro PRIVATE
    #     ${DDRAW_INCLUDE_DIR}
    #     ${DINPUT_INCLUDE_DIR}
    #     ${DSOUND_INCLUDE_DIR}
    #     ${DXGUID_INCLUDE_DIR}
    # )

    target_link_libraries(allegro PUBLIC
        kernel32
        user32
        gdi32
        comdlg32
        ole32
        ${DINPUT_LIBRARIES}
        ${DDRAW_LIBRARIES}
        ${DXGUID_LIBRARIES}
        winmm
        ${DSOUND_LIBRARIES}
    )
endif(WIN32)

# -- Mac OS X --

if(ALLEGRO_MACOSX)

    target_sources(allegro PRIVATE
        src/macosx/cadigi.m
        src/macosx/camidi.m
        src/macosx/drivers.m
        src/macosx/hidjoy.m
        src/macosx/hidman.m
        src/macosx/keybd.m
        src/macosx/pcpu.m
        src/macosx/quartz.m
        src/macosx/qzmouse.m
        src/macosx/system.m
        src/macosx/cocoagl.m
        src/macosx/cocoashared.m
        src/unix/ufile.c
        src/unix/utimer.c
        src/unix/uptimer.c
        src/unix/usystem.c
        src/unix/uthreads.c

        src/macosx/main.m
    )

    foreach(fw Cocoa IOKit CoreAudio AudioUnit AudioToolbox OpenGL CoreVideo)
        find_library(${fw}_FRAMEWORK ${fw})
        mark_as_advanced(${fw}_FRAMEWORK)
        target_link_libraries(allegro PUBLIC ${${fw}_FRAMEWORK})
    endforeach(fw)

endif(ALLEGRO_MACOSX)

# -- iOS --

if(IOS)
    target_include_directories(allegro PRIVATE ${IOS_PLATFORM_INCLUDE})
    target_include_directories(allegro PRIVATE "${SDK_PATH}/usr/include")
    target_sources(allegro PRIVATE 
        src/ios/idrivers.c
        src/ios/isound.c
        src/ios/ifile.c
        src/ios/igfx.c
        src/ios/ikey.c
        src/ios/isystem.c
        src/ios/imouse.c
        src/ios/itimer.c
        src/ios/ithreads.c
        src/misc/colconv.c
    )
	link_directories(IOS_PLATFORM_LIB)
endif(IOS)

# -- Android --

if(ANDROID)
    target_sources(allegro PRIVATE
        src/android/adrivers.c
        src/android/asound.c
        src/android/afile.c
        src/android/agfx.c
        src/android/akey.c
        src/android/asystem.c
        src/android/amouse.c
        src/android/ajni.c
        src/android/atimer.c
        src/misc/colconv.c
    )
endif(ANDROID)


#-----------------------------------------------------------------------------#
#
# Generate and install headers
#

configure_file(
    include/allegro/platform/alplatf.h.cmake
    ${CMAKE_BINARY_DIR}/include/allegro/platform/alplatf.h
    @ONLY
    )

if(UNIX)
    configure_file(
        include/allegro/platform/alunixac.h.cmake
        ${CMAKE_BINARY_DIR}/include/allegro/platform/alunixac.h
        )
endif(UNIX)

target_include_directories(allegro BEFORE PUBLIC ${CMAKE_BINARY_DIR}/include)
target_include_directories(allegro PUBLIC include)


#-----------------------------------------------------------------------------#
#
# Alias
#

add_library(Allegro::Allegro ALIAS allegro)

#-----------------------------------------------------------------------------#
# vim: set sts=4 sw=4 et:
