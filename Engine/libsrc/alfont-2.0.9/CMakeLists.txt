add_library(alfont)

set_target_properties(alfont PROPERTIES
    C_STANDARD 11
    C_EXTENSIONS NO
)

target_include_directories(alfont PUBLIC ../../../Common/libinclude)

target_sources(alfont 
    PRIVATE
    ../../../Common/libinclude/alfontdll.h
    ../../../Common/libinclude/alfont.h
    alfont.c
)

target_link_libraries(alfont PUBLIC Allegro::Allegro FreeType::FreeType)

add_library(AlFont::AlFont ALIAS alfont)
