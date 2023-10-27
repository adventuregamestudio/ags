
#-----------------------------------------------------------------------------#
#
# CMake setup
#

# Refer to https://gitlab.kitware.com/cmake/community/wikis/doc/cmake/Life-Cycle-Considerations
cmake_minimum_required(VERSION 3.7.2 FATAL_ERROR)


#-----------------------------------------------------------------------------#
#
# Build options
#

# Set the project name.
project(ALLEGRO C CXX)

if(WIN32)
   enable_language(RC OPTIONAL)
endif(WIN32)

set(ALLEGRO_VERSION 4.4.3)

# Search in the `cmake' directory for additional CMake modules.
set(CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake" ${CMAKE_MODULE_PATH})

# Put libraries into `lib'.
set(LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/lib)

# Lists of all the source files.
include(FileList)


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

test_big_endian(ALLEGRO_BIG_ENDIAN)
if(NOT ALLEGRO_BIG_ENDIAN)
   set(ALLEGRO_LITTLE_ENDIAN 1)
endif(NOT ALLEGRO_BIG_ENDIAN)

if(UNIX)

    check_include_files(dirent.h ALLEGRO_HAVE_DIRENT_H)
    check_include_files(inttypes.h ALLEGRO_HAVE_INTTYPES_H)
    check_include_files(stdint.h ALLEGRO_HAVE_STDINT_H)
    check_include_files(sys/io.h ALLEGRO_HAVE_SYS_IO_H)
    check_include_files(sys/stat.h ALLEGRO_HAVE_SYS_STAT_H)
    check_include_files(sys/time.h ALLEGRO_HAVE_SYS_TIME_H)
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

    set(ALLEGRO_WITH_MAGIC_MAIN 0)

    # XXX const
    # XXX inline
    # XXX size_t
endif(UNIX)

if(UNIX AND NOT (ANDROID OR EMSCRIPTEN))
    check_c_source_runs("
        static int notsupported = 1;
        void test_ctor (void) __attribute__((constructor));
        void test_ctor (void) { notsupported = 0; }
        int main(void) { return (notsupported); }
        "
            ALLEGRO_USE_CONSTRUCTOR)

    find_library(RT_LIBRARY rt)
    check_c_source_compiles("
        #include <time.h>
        int main(void) {
            struct timespec new_time_ns;
            clock_gettime(CLOCK_MONOTONIC, &new_time_ns);
            return 0;
        }"
            ALLEGRO_HAVE_POSIX_MONOTONIC_CLOCK
            )
endif()


if(ANDROID)
    set(ALLEGRO_USE_CONSTRUCTOR true)
    set(ALLEGRO_HAVE_POSIX_MONOTONIC_CLOCK true)
endif()

#-----------------------------------------------------------------------------#
#
# Compiler and platform setup
#

option(STRICT_WARN "Halt at warnings" off)

if(CMAKE_COMPILER_IS_GNUCC)
    set(COMPILER_GCC 1)
    set(ALLEGRO_GCC 1)
    set(WFLAGS "-W -Wall -Wno-unused-parameter")
    set(WFLAGS_C_ONLY "-Wdeclaration-after-statement")
    if(STRICT_WARN)
        set(WFLAGS "${WFLAGS} -Werror -Wpointer-arith")
        set(WFLAGS_C_ONLY "${WFLAGS_C_ONLY} -Wmissing-declarations")
        set(WFLAGS_C_ONLY "${WFLAGS_C_ONLY} -Wstrict-prototypes")
    endif(STRICT_WARN)
endif(CMAKE_COMPILER_IS_GNUCC)

if(MSVC)
    set(COMPILER_MSVC 1)
    set(ALLEGRO_MSVC 1)
    set(WFLAGS "/W3 -D_CRT_SECURE_NO_DEPRECATE -D_CRT_NONSTDC_NO_DEPRECATE")
endif()

if (${CMAKE_SYSTEM_NAME} MATCHES "Windows")
    # WIN32 is set by CMake for any Windows platform
    set(ALLEGRO_WINDOWS TRUE)
elseif (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    set(ALLEGRO_UNIX TRUE)
elseif (${CMAKE_SYSTEM_NAME} MATCHES "Android")
    set(ALLEGRO_ANDROID TRUE)
elseif (${CMAKE_SYSTEM_NAME} MATCHES "Emscripten")
    set(ALLEGRO_UNIX TRUE)
elseif (IOS)
    set(ALLEGRO_IOS TRUE)
elseif (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    set(ALLEGRO_MACOSX TRUE)
elseif (${CMAKE_SYSTEM_NAME} MATCHES "FreeBSD")
    set(ALLEGRO_UNIX TRUE)
else ()
    message(FATAL_ERROR "Unsupported system: ${CMAKE_SYSTEM_NAME}")
endif ()

message( "CMAKE_SYSTEM_NAME:" ${CMAKE_SYSTEM_NAME} )
message( "ALLEGRO_MSVC:" ${ALLEGRO_MSVC} )
message( "ALLEGRO_UNIX:" ${ALLEGRO_UNIX} )
message( "ALLEGRO_ANDROID:" ${ALLEGRO_ANDROID} )
message( "ALLEGRO_IOS:" ${ALLEGRO_IOS} )
message( "ALLEGRO_MACOSX:" ${ALLEGRO_MACOSX} )

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${WFLAGS} ${WFLAGS_C_ONLY}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${WFLAGS} ${WFLAGS_CXX_ONLY}")

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DALLEGRO_STATICLINK")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DALLEGRO_STATICLINK")

set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -DDEBUGMODE=1")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DDEBUGMODE=1")

list(APPEND CMAKE_BUILD_TYPES Profile)
mark_as_advanced(
    CMAKE_C_FLAGS_PROFILE
    CMAKE_CXX_FLAGS_PROFILE
    CMAKE_EXE_LINKER_FLAGS_PROFILE
    )
if(COMPILER_GCC)
    set(CMAKE_C_FLAGS_PROFILE "-pg"
        CACHE STRING "profiling flags")
    set(CMAKE_CXX_FLAGS_PROFILE "${CMAKE_C_FLAGS_PROFILE}"
        CACHE STRING "profiling flags")
    set(CMAKE_EXE_LINKER_FLAGS_PROFILE "-pg"
        CACHE STRING "profiling flags")
    set(CMAKE_SHARED_LINKER_FLAGS_PROFILE "-pg"
        CACHE STRING "profiling flags")
endif(COMPILER_GCC)
if(COMPILER_MSVC)
    set(CMAKE_C_FLAGS_PROFILE "-Gd -Ox"
        CACHE STRING "profiling flags")
    set(CMAKE_CXX_FLAGS_PROFILE "${CMAKE_C_FLAGS_PROFILE}"
        CACHE STRING "profiling flags")
    set(CMAKE_EXE_LINKER_FLAGS_PROFILE "-profile"
        CACHE STRING "profiling flags")
endif(COMPILER_MSVC)

#-----------------------------------------------------------------------------#

# Not sure if we want to support disabling these any more.
set(ALLEGRO_COLOR8 1)
set(ALLEGRO_COLOR16 1)
set(ALLEGRO_COLOR24 1)
set(ALLEGRO_COLOR32 1)

set(ALLEGRO_NO_ASM 1)



#-----------------------------------------------------------------------------#
#
# Platform drivers
#

set(PLATFORM_SOURCES)

if(ALLEGRO_WINDOWS)
   list(APPEND PLATFORM_SOURCES ${ALLEGRO_SRC_WIN_FILES})
else()
    # non-windows platforms are use unixy/posix file handling and have no gdi
   list(APPEND PLATFORM_SOURCES ${ALLEGRO_SRC_UNIX_FILES})
endif()



#-----------------------------------------------------------------------------#
#
# Main library
#

add_library(allegro 
    ${ALLEGRO_SRC_FILES}
    ${ALLEGRO_SRC_C_FILES}
    ${PLATFORM_SOURCES}
    ${ALLEGRO_PUBLIC_HEADERS}
)

set_target_properties(allegro
   PROPERTIES
   DEBUG_POSTFIX -debug
   PROFILE_POSTFIX -profile
)

set(allegro_OUTPUT_NAME alleg)

set_target_properties(allegro
    PROPERTIES
    COMPILE_FLAGS "-DALLEGRO_SRC"
    LINK_FLAGS "${ALLEGRO_LINK_FLAGS}"
    OUTPUT_NAME ${allegro_OUTPUT_NAME}
    VERSION ${ALLEGRO_VERSION}
    )

# Search for C header files in these directories.

message("${ALLEGRO_PUBLIC_HEADERS}")
target_include_directories(allegro 
   PRIVATE ${PROJECT_SOURCE_DIR}/include ${PROJECT_BINARY_DIR}/include
   PUBLIC ${CMAKE_CURRENT_BINARY_DIR}/include ${PROJECT_SOURCE_DIR}/include
)


#-----------------------------------------------------------------------------#
# vim: set sts=4 sw=4 et:
