#ifndef __AC_PLATFORM_H
#define __AC_PLATFORM_H

// platform definitions. Not intended for replacing types or checking for libraries.

// check Android first because sometimes it can get confused with host OS
#if defined(__ANDROID__) || defined(ANDROID)
    #define AGS_PLATFORM_OS_WINDOWS    (0)
    #define AGS_PLATFORM_OS_LINUX      (0)
    #define AGS_PLATFORM_OS_MACOS      (0)
    #define AGS_PLATFORM_OS_ANDROID    (1)
    #define AGS_PLATFORM_OS_IOS        (0)
    #define AGS_PLATFORM_OS_PSP        (0)
    #define AGS_PLATFORM_OS_EMSCRIPTEN (0)
    #define AGS_PLATFORM_OS_FREEBSD    (0)
#elif defined(_WIN32)
    //define something for Windows (32-bit and 64-bit)
    #define AGS_PLATFORM_OS_WINDOWS    (1)
    #define AGS_PLATFORM_OS_LINUX      (0)
    #define AGS_PLATFORM_OS_MACOS      (0)
    #define AGS_PLATFORM_OS_ANDROID    (0)
    #define AGS_PLATFORM_OS_IOS        (0)
    #define AGS_PLATFORM_OS_PSP        (0)
    #define AGS_PLATFORM_OS_EMSCRIPTEN (0)
    #define AGS_PLATFORM_OS_FREEBSD    (0)
#elif defined(__APPLE__)
    #include "TargetConditionals.h"
    #ifndef TARGET_OS_SIMULATOR
        #define TARGET_OS_SIMULATOR (0)
    #endif
    #ifndef TARGET_OS_IOS
        #define TARGET_OS_IOS (0)
    #endif
    #ifndef TARGET_OS_OSX
        #define TARGET_OS_OSX (0)
    #endif

    #if TARGET_OS_SIMULATOR || TARGET_IPHONE_SIMULATOR
        #define AGS_PLATFORM_OS_WINDOWS    (0)
        #define AGS_PLATFORM_OS_LINUX      (0)
        #define AGS_PLATFORM_OS_MACOS      (0)
        #define AGS_PLATFORM_OS_ANDROID    (0)
        #define AGS_PLATFORM_OS_IOS        (1)
        #define AGS_PLATFORM_OS_PSP        (0)
        #define AGS_PLATFORM_OS_EMSCRIPTEN (0)
        #define AGS_PLATFORM_OS_FREEBSD    (0)
    #elif TARGET_OS_IOS || TARGET_OS_IPHONE
        #define AGS_PLATFORM_OS_WINDOWS    (0)
        #define AGS_PLATFORM_OS_LINUX      (0)
        #define AGS_PLATFORM_OS_MACOS      (0)
        #define AGS_PLATFORM_OS_ANDROID    (0)
        #define AGS_PLATFORM_OS_IOS        (1)
        #define AGS_PLATFORM_OS_PSP        (0)
        #define AGS_PLATFORM_OS_EMSCRIPTEN (0)
        #define AGS_PLATFORM_OS_FREEBSD    (0)
    #elif TARGET_OS_OSX || TARGET_OS_MAC
        #define AGS_PLATFORM_OS_WINDOWS    (0)
        #define AGS_PLATFORM_OS_LINUX      (0)
        #define AGS_PLATFORM_OS_MACOS      (1)
        #define AGS_PLATFORM_OS_ANDROID    (0)
        #define AGS_PLATFORM_OS_IOS        (0)
        #define AGS_PLATFORM_OS_PSP        (0)
        #define AGS_PLATFORM_OS_EMSCRIPTEN (0)
        #define AGS_PLATFORM_OS_FREEBSD    (0)
    #else
        #error "Unknown Apple platform"
    #endif
#elif defined(__linux__)
    #define AGS_PLATFORM_OS_WINDOWS    (0)
    #define AGS_PLATFORM_OS_LINUX      (1)
    #define AGS_PLATFORM_OS_MACOS      (0)
    #define AGS_PLATFORM_OS_ANDROID    (0)
    #define AGS_PLATFORM_OS_IOS        (0)
    #define AGS_PLATFORM_OS_PSP        (0)
    #define AGS_PLATFORM_OS_EMSCRIPTEN (0)
    #define AGS_PLATFORM_OS_FREEBSD    (0)
#elif defined(__EMSCRIPTEN__)
    #define AGS_PLATFORM_OS_WINDOWS    (0)
    #define AGS_PLATFORM_OS_LINUX      (0)
    #define AGS_PLATFORM_OS_MACOS      (0)
    #define AGS_PLATFORM_OS_ANDROID    (0)
    #define AGS_PLATFORM_OS_IOS        (0)
    #define AGS_PLATFORM_OS_PSP        (0)
    #define AGS_PLATFORM_OS_EMSCRIPTEN (1)
    #define AGS_PLATFORM_OS_FREEBSD    (0)
#elif defined(__FreeBSD__)
    #define AGS_PLATFORM_OS_WINDOWS    (0)
    #define AGS_PLATFORM_OS_LINUX      (0)
    #define AGS_PLATFORM_OS_MACOS      (0)
    #define AGS_PLATFORM_OS_ANDROID    (0)
    #define AGS_PLATFORM_OS_IOS        (0)
    #define AGS_PLATFORM_OS_PSP        (0)
    #define AGS_PLATFORM_OS_EMSCRIPTEN (0)
    #define AGS_PLATFORM_OS_FREEBSD    (1)
#else
    #error "Unknown platform"
#endif

#if defined(__LP64__)
    // LP64 machine, OS X or Linux
    // int 32bit | long 64bit | long long 64bit | void* 64bit
    #define AGS_PLATFORM_64BIT (1)
#elif defined(_WIN64)
    // LLP64 machine, Windows
    // int 32bit | long 32bit | long long 64bit | void* 64bit
    #define AGS_PLATFORM_64BIT (1)
#else
    // 32-bit machine, Windows or Linux or OS X
    // int 32bit | long 32bit | long long 64bit | void* 32bit
    #define AGS_PLATFORM_64BIT (0)
#endif

#if defined(_WIN32)
    #define AGS_PLATFORM_ENDIAN_LITTLE  (1)
    #define AGS_PLATFORM_ENDIAN_BIG     (0)
#elif __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    #define AGS_PLATFORM_ENDIAN_LITTLE  (1)
    #define AGS_PLATFORM_ENDIAN_BIG     (0)
#elif __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
    #define AGS_PLATFORM_ENDIAN_LITTLE  (0)
    #define AGS_PLATFORM_ENDIAN_BIG     (1)
#else
    #error "Unknown platform"
#endif

#if defined(__MINGW32__)
#define AGS_PLATFORM_WINDOWS_MINGW (1)
#else
#define AGS_PLATFORM_WINDOWS_MINGW (0)
#endif

#if defined(_DEBUG)
    #define AGS_PLATFORM_DEBUG  (1)
#elif ! defined(NDEBUG)
    #define AGS_PLATFORM_DEBUG  (1)
#else
    #define AGS_PLATFORM_DEBUG  (0)
#endif

// Certain methods or classes may be meant for the code testing purposes only
#if !defined(AGS_PLATFORM_TEST)
    #define AGS_PLATFORM_TEST (0)
#endif
// This option surrounds the test code which requires write disk access
#if !defined(AGS_PLATFORM_TEST_FILE_IO)
    #define AGS_PLATFORM_TEST_FILE_IO (AGS_PLATFORM_TEST)
#endif

#define AGS_PLATFORM_DESKTOP (AGS_PLATFORM_OS_WINDOWS || \
                        AGS_PLATFORM_OS_LINUX   || \
                        AGS_PLATFORM_OS_FREEBSD || \
                        AGS_PLATFORM_OS_MACOS)
#define AGS_PLATFORM_MOBILE ((AGS_PLATFORM_OS_ANDROID) || (AGS_PLATFORM_OS_IOS))

#define AGS_HAS_DIRECT3D (AGS_PLATFORM_OS_WINDOWS)
#define AGS_HAS_OPENGL (AGS_PLATFORM_OS_WINDOWS    || \
                        AGS_PLATFORM_OS_ANDROID    || \
                        AGS_PLATFORM_OS_IOS        || \
                        AGS_PLATFORM_OS_LINUX      || \
                        AGS_PLATFORM_OS_EMSCRIPTEN || \
                        AGS_PLATFORM_OS_FREEBSD    || \
                        AGS_PLATFORM_OS_MACOS)
#define AGS_OPENGL_ES2 (AGS_PLATFORM_OS_ANDROID    || \
                        AGS_PLATFORM_OS_EMSCRIPTEN || \
                        AGS_PLATFORM_OS_IOS)

// Only allow searching around for game data on desktop systems;
// otherwise use explicit argument either from program wrapper, command-line
// or read from default config.
#define AGS_SEARCH_FOR_GAME_ON_LAUNCH (AGS_PLATFORM_OS_WINDOWS    || \
                                       AGS_PLATFORM_OS_LINUX      || \
                                       AGS_PLATFORM_OS_MACOS      || \
                                       AGS_PLATFORM_OS_EMSCRIPTEN || \
                                       AGS_PLATFORM_OS_FREEBSD )

#define AGS_PLATFORM_IS_XDG_UNIX (AGS_PLATFORM_OS_LINUX || AGS_PLATFORM_OS_FREEBSD)

#if !defined(DEBUG_MANAGED_OBJECTS)
    #define DEBUG_MANAGED_OBJECTS (0)
#endif

#if !defined(DEBUG_SPRITECACHE)
    #define DEBUG_SPRITECACHE (0)
#endif

#endif // __AC_PLATFORM_H
