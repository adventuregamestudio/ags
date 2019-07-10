cmake_minimum_required(VERSION 3.14)
project(ogg LANGUAGES C)

include(CheckIncludeFiles)

# Configure config_type.h
check_include_files(inttypes.h INCLUDE_INTTYPES_H)
check_include_files(stdint.h INCLUDE_STDINT_H)
check_include_files(sys/types.h INCLUDE_SYS_TYPES_H)

set(SIZE16 int16_t)
set(USIZE16 uint16_t)
set(SIZE32 int32_t)
set(USIZE32 uint32_t)
set(SIZE64 int64_t)
set(USIZE64 uint64_t)

configure_file(include/ogg/config_types.h.in ${CMAKE_CURRENT_BINARY_DIR}/include/ogg/config_types.h @ONLY)


add_library(ogg)

target_sources(ogg PRIVATE
    src/bitwise.c
    src/framing.c
    ${CMAKE_CURRENT_BINARY_DIR}/include/ogg/config_types.h
    include/ogg/ogg.h
    include/ogg/os_types.h
)

target_include_directories(ogg PUBLIC ${CMAKE_CURRENT_LIST_DIR}/include ${CMAKE_CURRENT_BINARY_DIR}/include)


add_library(Ogg::Ogg ALIAS ogg)