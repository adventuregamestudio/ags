add_library(alogg)

set_target_properties(alogg PROPERTIES
    C_STANDARD 11
    C_EXTENSIONS NO
)

target_include_directories(alogg PUBLIC ../../../Common/libinclude)

target_sources(alogg 
    PRIVATE
    ../../../Common/libinclude/alogg.h
    ../../../Common/libinclude/aloggdll.h
    alogg.c
)

target_link_libraries(alogg PUBLIC Allegro::Allegro AGS::Common Vorbis::Vorbis Vorbis::VorbisFile)

add_library(AlOgg::AlOgg ALIAS alogg)
