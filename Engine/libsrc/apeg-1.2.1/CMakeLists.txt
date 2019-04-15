add_library(apeg)

set_target_properties(apeg PROPERTIES
    C_STANDARD 11
    C_EXTENSIONS NO
)

target_include_directories(apeg PUBLIC . )

target_sources(apeg 
    PRIVATE
    adisplay.c
    getbits.c
    getblk.c
    gethdr.c
    getpic.c
    idct.c
    motion.c
    mpeg1dec.c
    ogg.c
    recon.c
    audio/aaudio.c
    audio/mpg123.c

    # if MPEG_AUDIO not disabled
    # audio/apegcommon.c
    # audio/dct64.c
    # audio/decode_1to1.c
    # audio/decode_2to1.c
    # audio/decode_4to1.c
    # audio/layer1.c
    # audio/layer2.c
    # audio/layer3.c
    # audio/readers.c
    # audio/tabinit.c
    # audio/vbrhead.c
)

target_compile_definitions(apeg PUBLIC DISABLE_MPEG_AUDIO)

target_link_libraries(apeg PUBLIC Allegro::Allegro Ogg::Ogg Vorbis::Vorbis Vorbis::VorbisFile Theora::TheoraDecoder)

add_library(Apeg::Apeg ALIAS apeg)
