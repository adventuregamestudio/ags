add_library(almp3)

set_target_properties(almp3 PROPERTIES
    C_STANDARD 11
    C_EXTENSIONS NO
)

target_include_directories(almp3 
    PUBLIC  include
    PRIVATE .           # for ./decoder
)

target_sources(almp3 
    PRIVATE
    src/almp3.c
    decoder/common.c
    decoder/dct64_i386.c
    decoder/decode_i386.c
    decoder/huffman.h
    decoder/interface.c
    decoder/l2tables.h
    decoder/layer2.c
    decoder/layer3.c
    decoder/mpg123.h
    decoder/mpglib.h
    decoder/tabinit.c
)

target_link_libraries(almp3 PUBLIC Allegro::Allegro)

# The PSP (and embedded processors in general) only has hardware support for single precision math.
if (ANDROID OR IOS)
    target_compile_definitions(almp3 PRIVATE REAL_IS_FLOAT)
endif()

add_library(AlMp3::AlMp3 ALIAS almp3)
