cmake_minimum_required(VERSION 3.13..3.14)

set(CMAKE_USER_MAKE_RULES_OVERRIDE ${CMAKE_CURRENT_SOURCE_DIR}/CMake/c_flag_overrides.cmake)
set(CMAKE_USER_MAKE_RULES_OVERRIDE_CXX ${CMAKE_CURRENT_SOURCE_DIR}/CMake/cxx_flag_overrides.cmake)

project(AGS
    VERSION 3.5.0.8
    LANGUAGES CXX C)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/CMake")

option(AGS_NO_MP3_PLAYER "Disable MP3" OFF)
option(AGS_BUILTIN_PLUGINS "Built in plugins" OFF)
set(AGS_BUILD_STR "" CACHE STRING "Engine Build Information")

include(FetchContent)
set(FETCHCONTENT_UPDATES_DISCONNECTED on)

# todo use platform id (from generators)
if(UNIX AND NOT APPLE)
    set(LINUX TRUE)
endif()

if(UNIX AND APPLE)
    set(MACOS TRUE)
endif()

if (WIN32 AND NOT CMAKE_SIZEOF_VOID_P EQUAL 4)
	message(FATAL_ERROR "Windows builds only support 32bit for now")
endif()


include(CheckIPOSupported)
check_ipo_supported(RESULT ipo_supported OUTPUT ipo_not_supported_reason)
if(ipo_supported)
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
else()
    message(STATUS "Interprocedural optimisation (IPO/LTO) not supported: <${ipo_not_supported_reason}>")
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

if (WIN32)
    find_package(DirectX)
elseif (LINUX)
    find_package(X11)
endif()

include(FetchAllegro)
include(FetchOgg)
include(FetchVorbis)
include(FetchTheora)

add_subdirectory(Common/libsrc/aastr-0.1.1      EXCLUDE_FROM_ALL)
add_subdirectory(Common                         EXCLUDE_FROM_ALL)

add_subdirectory(Engine/libsrc/alfont-2.0.9     EXCLUDE_FROM_ALL)
add_subdirectory(Engine/libsrc/almp3-2.0.5      EXCLUDE_FROM_ALL)
add_subdirectory(Engine/libsrc/alogg            EXCLUDE_FROM_ALL)
add_subdirectory(Engine/libsrc/apeg-1.2.1       EXCLUDE_FROM_ALL)
add_subdirectory(Engine/libsrc/dumb-0.9.2       EXCLUDE_FROM_ALL)
add_subdirectory(Engine/libsrc/freetype-2.1.3   EXCLUDE_FROM_ALL)
add_subdirectory(Engine/libsrc/glad             EXCLUDE_FROM_ALL)
add_subdirectory(Engine/libsrc/hq2x             EXCLUDE_FROM_ALL)
add_subdirectory(Engine/libsrc/libcda-0.5       EXCLUDE_FROM_ALL)

add_subdirectory(Engine)
