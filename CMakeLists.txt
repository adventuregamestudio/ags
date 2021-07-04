cmake_minimum_required(VERSION 3.13..3.14)

set(CMAKE_USER_MAKE_RULES_OVERRIDE ${CMAKE_CURRENT_SOURCE_DIR}/CMake/c_flag_overrides.cmake)
set(CMAKE_USER_MAKE_RULES_OVERRIDE_CXX ${CMAKE_CURRENT_SOURCE_DIR}/CMake/cxx_flag_overrides.cmake)
set(CMAKE_OSX_DEPLOYMENT_TARGET "10.9" CACHE STRING "Minimum OS X deployment version")

project(AGS
    VERSION 3.6.0.4
    LANGUAGES CXX C)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/CMake")

option(AGS_TESTS "Build tests" OFF)
option(AGS_BUILD_COMPILER "Build compiler library" OFF)
option(AGS_NO_MP3_PLAYER "Disable MP3" OFF)
option(AGS_NO_VIDEO_PLAYER "Disable Video" OFF)
option(AGS_BUILTIN_PLUGINS "Built in plugins" ON)
set(AGS_BUILD_STR "" CACHE STRING "Engine Build Information")


message("------ AGS selected CMake options ------")
message(" AGS_TESTS: ${AGS_TESTS}")
message(" AGS_BUILD_COMPILER: ${AGS_BUILD_COMPILER}")
message(" AGS_NO_MP3_PLAYER: ${AGS_NO_MP3_PLAYER}")
message(" AGS_NO_VIDEO_PLAYER: ${AGS_NO_VIDEO_PLAYER}")
message(" AGS_BUILTIN_PLUGINS: ${AGS_BUILTIN_PLUGINS}")
message("----------------------------------------")

# Tools like ninja don't have a pseudo-terminal so compilers assume no coloured output
option (AGS_FORCE_COLOURED_OUTPUT "Always produce ANSI-coloured output (GNU/Clang only)." OFF)
if (${AGS_FORCE_COLOURED_OUTPUT})
    if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
        add_compile_options (-fdiagnostics-color=always)
    elseif ("${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang")
        add_compile_options (-fcolor-diagnostics)
    endif ()
endif ()

include(FetchContent)
set(FETCHCONTENT_UPDATES_DISCONNECTED on)

if (${CMAKE_SYSTEM_NAME} MATCHES "Windows")
    # WIN32 is set by CMake for any Windows platform
elseif (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    set(LINUX TRUE)
elseif (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    set(MACOS TRUE)
elseif (${CMAKE_SYSTEM_NAME} MATCHES "Android")
    set(ANDROID TRUE)
elseif (${CMAKE_SYSTEM_NAME} MATCHES "FreeBSD")
    set(FREEBSD TRUE)
else()
    message(FATAL_ERROR "Unsupported system: ${CMAKE_SYSTEM_NAME}")
endif ()

message( "CMAKE_SYSTEM_NAME:" ${CMAKE_SYSTEM_NAME} )
message( "WIN32:" ${WIN32} )
message( "LINUX:" ${LINUX} )
message( "MACOS:" ${MACOS} )
message( "ANDROID:" ${ANDROID} )
message( "FREEBSD:" ${FREEBSD} )
message("----------------------------------------")

if (WIN32 AND NOT CMAKE_SIZEOF_VOID_P EQUAL 4)
	message(FATAL_ERROR "Windows builds only support 32bit for now")
endif()

if(LINUX)
    if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
        if (CMAKE_COMPILER_IS_GNUCC AND CMAKE_CXX_COMPILER_VERSION VERSION_GREATER 9.0)
            # without no-pie we can't double-click run ags in Nautilus
            # this should only be needed in newer gcc versions
            # set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fno-pie -no-pie")
        endif ()
    endif ()
endif (LINUX)

if(ANDROID)
    set(AGS_OPENGLES2 TRUE)
endif(ANDROID)

if(AGS_TESTS)
    add_compile_definitions("AGS_PLATFORM_TEST=1") #make sure it's test even on Release
endif()

# Unfortunately there's an upstream bug in Android NDK with Clang
# https://github.com/android/ndk/issues/721
# https://reviews.llvm.org/D79919
# The issue has been fixed upstream but hasn't been rolled in Android Studio yet
# Once it gets fixed, we can remove the Android special case.
if(NOT ANDROID)
    include(CheckIPOSupported)
    check_ipo_supported(RESULT ipo_supported OUTPUT ipo_not_supported_reason)
    if(ipo_supported)
        if(${CMAKE_BUILD_TYPE} MATCHES Release)
            set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELEASE          TRUE)
            set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELWITHDEBINFO   TRUE)
        endif()
    else()
        message(STATUS "Interprocedural optimisation (IPO/LTO) not supported: <${ipo_not_supported_reason}>")
    endif()
else()
    # it is Android NDK, so we apply LTO on Release Builds
    if(${CMAKE_BUILD_TYPE} MATCHES Release)
        set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3 -flto=full")
    endif()
endif()

add_compile_definitions("_FILE_OFFSET_BITS=64")

add_compile_definitions("$<$<CONFIG:DEBUG>:_DEBUG>")
add_compile_definitions("$<$<CONFIG:RELEASE>:NDEBUG>")

if(MSVC)
    add_compile_options(/MP)    # Build with Multiple Processes
    add_compile_definitions(_CRT_SECURE_NO_DEPRECATE)
    add_compile_definitions(_CRT_NONSTDC_NO_DEPRECATE)
else()
    add_compile_options(
        -fsigned-char 
        -fno-strict-aliasing 
        -fwrapv

        -Wall
        -Wextra

        -Wendif-labels
        -Wfloat-equal
        -Wformat
        -Wformat-security
        -Winit-self
        -Winline
        -Wmissing-noreturn
        -Wpointer-arith
        -Wshadow
        -Wundef
        -Wwrite-strings
        -Wunused-result

        # probably need fixing but disable until we have time
        -Wno-unknown-pragmas
        -Wno-deprecated-declarations
        -Wno-unused-parameter
        -Wno-sign-compare
        -Wno-cast-align
        -Wno-cast-qual
        -Wno-missing-declarations
        -Wno-switch-enum
        # -Wlarger-than-4096
        -Wno-redundant-decls

        -Werror=write-strings
        #-Werror=implicit-function-declaration
        #-Werror=unused-result
    )

    add_compile_options($<$<COMPILE_LANGUAGE:CXX>:-Wno-old-style-cast>)
    add_compile_options($<$<COMPILE_LANGUAGE:CXX>:-Werror=delete-non-virtual-dtor>)

    add_compile_options($<$<COMPILE_LANGUAGE:C>:-Wbad-function-cast>)
    add_compile_options($<$<COMPILE_LANGUAGE:C>:-Wdeclaration-after-statement>)
    add_compile_options($<$<COMPILE_LANGUAGE:C>:-Wno-missing-prototypes>)
    add_compile_options($<$<COMPILE_LANGUAGE:C>:-Wold-style-definition>)
    add_compile_options($<$<COMPILE_LANGUAGE:C>:-Wstrict-prototypes>)

endif()

find_package(PkgConfig)
find_package(Threads)

if(AGS_TESTS)
    include(FetchGoogleTest)
    enable_testing()
endif()

include(FetchSDL2)
include(FetchSDL_Sound)

include(FindLocalOpenAL)
include(FindLocalAllegro)

include(FetchOgg)
include(FetchVorbis)
include(FetchTheora)

add_subdirectory(libsrc/glm                     EXCLUDE_FROM_ALL)

add_subdirectory(Common/libsrc/aastr-0.1.1      EXCLUDE_FROM_ALL)
add_subdirectory(Common/libsrc/alfont-2.0.9     EXCLUDE_FROM_ALL)
add_subdirectory(Common/libsrc/freetype-2.1.3   EXCLUDE_FROM_ALL)
add_subdirectory(Common)

add_subdirectory(Engine/libsrc/apeg-1.2.1       EXCLUDE_FROM_ALL)
if(AGS_OPENGLES2)
    add_subdirectory(Engine/libsrc/glad-gles2       EXCLUDE_FROM_ALL)
else()
    add_subdirectory(Engine/libsrc/glad             EXCLUDE_FROM_ALL)
endif()
add_subdirectory(Engine/libsrc/libcda-0.5       EXCLUDE_FROM_ALL)

add_subdirectory(Engine)
if(AGS_BUILD_COMPILER)
    add_subdirectory(Compiler)
endif()

set_property(DIRECTORY ${PROJECT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ags)
