cmake_minimum_required(VERSION 3.14)
project(theora LANGUAGES C)

add_library(theoradec)

target_include_directories(theoradec PUBLIC include)

target_sources(theoradec PRIVATE
    include/theora/codec.h
    include/theora/theora.h
    include/theora/theoradec.h
    include/theora/theoraenc.h

    lib/apiwrapper.c
    lib/bitpack.c
    lib/dequant.c
    lib/fragment.c
    lib/idct.c
    lib/info.c
    lib/internal.c
    lib/state.c
    lib/quant.c

    lib/decapiwrapper.c
    lib/decinfo.c
    lib/decode.c
    lib/huffdec.c
)

target_link_libraries(theoradec ogg)

add_library (Theora::TheoraDecoder ALIAS theoradec)