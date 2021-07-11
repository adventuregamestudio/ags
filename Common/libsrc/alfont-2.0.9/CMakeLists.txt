add_library(alfont)

set_target_properties(alfont PROPERTIES
    C_STANDARD 11
    C_EXTENSIONS NO
)

target_include_directories(alfont PUBLIC . )

target_sources(alfont 
    PRIVATE
    alfont.h
    alfont.c
)

target_link_libraries(alfont PUBLIC Allegro::Allegro ${FREETYPE_LIBRARIES})

add_library(AlFont::AlFont ALIAS alfont)
