cmake_minimum_required(VERSION 3.14)
project(vorbis LANGUAGES C)

include(CheckIncludeFiles)

add_library(vorbis)

target_include_directories(vorbis
    PUBLIC include
    PRIVATE lib
)

target_sources(vorbis PRIVATE
    lib/envelope.h
    lib/lpc.h
    lib/lsp.h
    lib/codebook.h
    lib/misc.h
    lib/psy.h
    lib/masking.h
    lib/os.h
    lib/mdct.h
    lib/smallft.h
    lib/highlevel.h
    lib/registry.h
    lib/scales.h
    lib/window.h
    lib/lookup.h
    lib/lookup_data.h
    lib/codec_internal.h
    lib/backends.h
    lib/bitrate.h
    lib/mdct.c
    lib/smallft.c
    lib/block.c
    lib/envelope.c
    lib/window.c
    lib/lsp.c
    lib/lpc.c
    lib/analysis.c
    lib/synthesis.c
    lib/psy.c
    lib/info.c
    lib/floor1.c
    lib/floor0.c
    lib/res0.c
    lib/mapping0.c
    lib/registry.c
    lib/codebook.c
    lib/sharedbook.c
    lib/lookup.c
    lib/bitrate.c
)

target_link_libraries(vorbis ogg)

add_library(Vorbis::Vorbis ALIAS vorbis)




add_library(vorbisfile)

target_sources(vorbisfile PRIVATE
    lib/vorbisfile.c
)
 
target_include_directories(vorbisfile
    PUBLIC include
    PRIVATE lib
)

target_link_libraries(vorbisfile ogg vorbis)

add_library(Vorbis::VorbisFile ALIAS vorbisfile)
