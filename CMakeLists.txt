cmake_minimum_required(VERSION 3.13..3.14)

# keep fetch content working on CMake 3.X versions, but won't work in CMake 4.X
if(POLICY CMP0169)
    cmake_policy(SET CMP0169 OLD)
endif()

set(CMAKE_USER_MAKE_RULES_OVERRIDE ${CMAKE_CURRENT_SOURCE_DIR}/CMake/c_flag_overrides.cmake)
set(CMAKE_USER_MAKE_RULES_OVERRIDE_CXX ${CMAKE_CURRENT_SOURCE_DIR}/CMake/cxx_flag_overrides.cmake)
set(CMAKE_OSX_DEPLOYMENT_TARGET "10.9" CACHE STRING "Minimum OS X deployment version")

project(AGS
    VERSION 3.6.3.0
    LANGUAGES CXX C)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/CMake")

option(AGS_USE_LOCAL_ALL_LIBRARIES "Use a locally installed libraries" OFF)
option(AGS_USE_LOCAL_SDL2 "Use a locally installed SDL2" ${AGS_USE_LOCAL_ALL_LIBRARIES})
option(AGS_USE_LOCAL_SDL2_SOUND "Use a locally installed SDL2 Sound" ${AGS_USE_LOCAL_ALL_LIBRARIES})
option(AGS_USE_LOCAL_OGG "Use a locally installed OGG" ${AGS_USE_LOCAL_ALL_LIBRARIES})
option(AGS_USE_LOCAL_THEORA "Use a locally installed Theora" ${AGS_USE_LOCAL_ALL_LIBRARIES})
option(AGS_USE_LOCAL_VORBIS "Use a locally installed Vorbis" ${AGS_USE_LOCAL_ALL_LIBRARIES})

option(AGS_TESTS "Build tests" OFF)
option(AGS_BUILD_ENGINE "Build Engine" ON)
option(AGS_BUILD_TOOLS "Build Tools" OFF)
option(AGS_BUILD_COMPILER "Build compiler" ${AGS_BUILD_TOOLS})
option(AGS_NO_VIDEO_PLAYER "Disable Video" OFF)
option(AGS_BUILTIN_PLUGINS "Built in plugins" ON)
option(AGS_DEBUG_MANAGED_OBJECTS "Managed Objects Log" OFF)
option(AGS_DEBUG_SPRITECACHE "Sprite Cache Log" OFF)
set(AGS_BUILD_STR "" CACHE STRING "Engine Build Information")


message("------- AGS dependencies options -------")
message(" AGS_USE_LOCAL_ALL_LIBRARIES: ${AGS_USE_LOCAL_ALL_LIBRARIES}")
message(" AGS_USE_LOCAL_SDL2: ${AGS_USE_LOCAL_SDL2}")
message(" AGS_USE_LOCAL_SDL2_SOUND: ${AGS_USE_LOCAL_SDL2_SOUND}")
message(" AGS_USE_LOCAL_OGG: ${AGS_USE_LOCAL_OGG}")
message(" AGS_USE_LOCAL_THEORA: ${AGS_USE_LOCAL_THEORA}")
message(" AGS_USE_LOCAL_VORBIS: ${AGS_USE_LOCAL_VORBIS}")
message("------ AGS selected CMake options ------")
message(" AGS_TESTS: ${AGS_TESTS}")
message(" AGS_BUILD_ENGINE: ${AGS_BUILD_ENGINE}")
message(" AGS_BUILD_TOOLS: ${AGS_BUILD_TOOLS}")
message(" AGS_BUILD_COMPILER: ${AGS_BUILD_COMPILER}")
message(" AGS_NO_VIDEO_PLAYER: ${AGS_NO_VIDEO_PLAYER}")
message(" AGS_BUILTIN_PLUGINS: ${AGS_BUILTIN_PLUGINS}")
message(" AGS_DEBUG_MANAGED_OBJECTS: ${AGS_DEBUG_MANAGED_OBJECTS}")
message("----------------------------------------")

if(AGS_USE_LOCAL_SDL2)
    message(WARNING "Your local SDL2 may require SDL2.dll or other dynamic library to be installed.")
endif()

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
elseif (${CMAKE_SYSTEM_NAME} MATCHES "Emscripten")
    set(EMSCRIPTEN TRUE)
elseif (${CMAKE_SYSTEM_NAME} MATCHES "FreeBSD")
    set(FREEBSD TRUE)
else()
    message(FATAL_ERROR "Unsupported system: ${CMAKE_SYSTEM_NAME}")
endif ()

if(LINUX OR MACOS OR WIN32 OR FREEBSD)
    set(AGS_DESKTOP TRUE)
endif()

message( "CMAKE_SYSTEM_NAME:" ${CMAKE_SYSTEM_NAME} )
message( "WIN32:" ${WIN32} )
message( "LINUX:" ${LINUX} )
message( "MACOS:" ${MACOS} )
message( "ANDROID:" ${ANDROID} )
message( "EMSCRIPTEN:" ${EMSCRIPTEN} )
message( "FREEBSD:" ${FREEBSD} )
message("----------------------------------------")
message( "AGS_DESKTOP:" ${AGS_DESKTOP} )
message("----------------------------------------")

if (WIN32 AND NOT CMAKE_SIZEOF_VOID_P EQUAL 4)
	message(WARNING "Windows builds only support 32bit for now, this is experimental.")
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
    if(EMSCRIPTEN)
        set(ipo_supported FALSE)
    endif()

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

if(EMSCRIPTEN)
    set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/CMake/Emscripten")

    set(AGS_OPENGLES2 TRUE)
    set(USE_FLAGS " -sUSE_SDL=2 -sUSE_OGG=1 -sUSE_VORBIS=1 ")
    set(AGS_USE_LOCAL_OGG TRUE)
    set(AGS_USE_LOCAL_VORBIS TRUE)

    set(AGS_DISABLE_THREADS TRUE)
    if(AGS_DISABLE_THREADS)
        add_compile_definitions("AGS_DISABLE_THREADS=1")
        set(EMSDK_FLAGS_LINKER "\
        -s ALLOW_MEMORY_GROWTH=1 ")
    else()
        set(EMSDK_FLAGS_COMPILER "\
        -pthread \
        ")
        set(EMSDK_FLAGS_LINKER "\
        -s INITIAL_MEMORY=536870912 ")
    endif()

    set(EMSDK_FLAGS_COMPILER "${EMSDK_FLAGS_COMPILER} \
        -O3 \
        -s DISABLE_EXCEPTION_CATCHING=0 \
        ")

    set(EMSDK_FLAGS_LINKER "${EMSDK_FLAGS_LINKER} \
        -s ASYNCIFY \
        -s ALLOW_TABLE_GROWTH=1 \
        -s WASM=1 \
        -s INVOKE_RUN=0 \
		-s EXIT_RUNTIME=1  \
		-s EMULATE_FUNCTION_POINTER_CASTS=1 ")

    # set(EMSDK_DEBUG_CFLAGS "-gsource-map -fsanitize=null -fsanitize-minimal-runtime ")
    # set(EMSDK_DEBUG_LINKER_FLAGS "--emit-symbol-map --source-map-base http://127.0.0.1:8000/../ ")

    set(EMSDK_FLAGS_LINKER "${EMSDK_FLAGS_LINKER} \
        -s FULL_ES2=1 \
        -s FORCE_FILESYSTEM=1 -lidbfs.js \
        --post-js ${CMAKE_SOURCE_DIR}/Emscripten/post.js \
        -s EXPORTED_FUNCTIONS=['_main','_ext_syncfs_done','_ext_toggle_fullscreen','_ext_get_windowed','_ext_gfxmode_get_width','_ext_gfxmode_get_height'] \
        -s EXPORTED_RUNTIME_METHODS=['ccall','callMain']")

    set(EMSDK_FLAGS_LINKER "${EMSDK_FLAGS_LINKER} ${EMSDK_DEBUG_LINKER_FLAGS} --shell-file ${CMAKE_SOURCE_DIR}/Emscripten/launcher_index.html ")

    set(CMAKE_EXECUTABLE_SUFFIX ".html")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${USE_FLAGS} ${EMSDK_FLAGS_COMPILER} ${EMSDK_DEBUG_CFLAGS}")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${USE_FLAGS} ${EMSDK_FLAGS_COMPILER} ${EMSDK_DEBUG_CFLAGS}")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${USE_FLAGS} ${EMSDK_FLAGS_LINKER}")

    add_compile_options(-fexceptions)

    include_directories(${CMAKE_SOURCE_DIR}/include ${SDL2_INCLUDE_DIRS} ${FREETYPE_INCLUDE_DIRS} ${VORBIS_INCLUDE_DIRS} ${OGG_INCLUDE_DIRS})

    message( "Emscripten configured\nCMAKE_CXX_FLAGS: ${CMAKE_CXX_FLAGS}\nCMAKE_C_FLAGS: ${CMAKE_C_FLAGS}\nCMAKE_EXE_LINKER_FLAGS: ${CMAKE_EXE_LINKER_FLAGS}\n" )
endif(EMSCRIPTEN)

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

###############################################################################
# dependencies we download the source or not depending on settings

if(NOT AGS_USE_LOCAL_SDL2 AND NOT EMSCRIPTEN)
    include(FetchSDL2)
else()
    find_package(SDL2 REQUIRED)
    include_directories("${SDL2_INCLUDE_DIRS}")
endif()

if(NOT AGS_USE_LOCAL_SDL2_SOUND)
    include(FetchSDL_Sound)
else()
    find_package(SDL2_sound REQUIRED)
endif()

if(AGS_USE_LOCAL_OGG)
    find_package(Ogg REQUIRED)
else()
    include(FetchOgg)
endif()

if (AGS_USE_LOCAL_VORBIS)
    find_package(Vorbis REQUIRED)
else()
    include(FetchVorbis)
endif()

if (AGS_USE_LOCAL_THEORA)
    find_package(Theora REQUIRED)
else()
    include(FetchTheora)
endif()

###############################################################################
# subdirectories of this project

include(FindLocalOpenAL)
include(FindLocalAllegro)

if (NOT AGS_USE_SYSTEM_GLM)
add_subdirectory(libsrc/glm                     EXCLUDE_FROM_ALL)
else()
    find_package(glm REQUIRED)
endif()
if (NOT AGS_USE_SYSTEM_TINYXML2)
add_subdirectory(libsrc/tinyxml2                EXCLUDE_FROM_ALL)
else()
    find_package(tinyxml2 REQUIRED)
endif()
if (NOT AGS_USE_SYSTEM_MINIZ)
add_subdirectory(libsrc/miniz                   EXCLUDE_FROM_ALL)
else()
    include(FindMiniz)
endif()

add_subdirectory(Common/libsrc/aastr-0.1.1      EXCLUDE_FROM_ALL)

if (NOT AGS_USE_SYSTEM_FREETYPE)
add_subdirectory(Common/libsrc/freetype-2.1.3   EXCLUDE_FROM_ALL)
set(FREETYPE_LIBRARIES FreeType::FreeType)
else()
    include(FindFreetype)
    include_directories("${FREETYPE_INCLUDE_DIRS}")
endif()

add_subdirectory(Common/libsrc/alfont-2.0.9     EXCLUDE_FROM_ALL)
add_subdirectory(Common)

add_subdirectory(Engine/libsrc/apeg-1.2.1       EXCLUDE_FROM_ALL)
if(AGS_OPENGLES2)
    add_subdirectory(Engine/libsrc/glad-gles2       EXCLUDE_FROM_ALL)
else()
    add_subdirectory(Engine/libsrc/glad             EXCLUDE_FROM_ALL)
endif()
add_subdirectory(Engine/libsrc/libcda-0.5       EXCLUDE_FROM_ALL)

if(AGS_BUILD_COMPILER)
    add_subdirectory(Compiler)
endif()

if(AGS_BUILD_TOOLS)
    add_subdirectory(Tools)
endif()

if(AGS_BUILD_ENGINE)
    add_subdirectory(Engine)
    set_property(DIRECTORY ${PROJECT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ags)
endif()
