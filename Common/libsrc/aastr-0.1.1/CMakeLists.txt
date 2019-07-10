add_library(aastr)

set_target_properties(aastr PROPERTIES
    C_STANDARD 11
    C_EXTENSIONS NO
)

target_include_directories(aastr PUBLIC .)

target_sources(aastr 
    PRIVATE
    aarot.c
    aastr.c
    aastr.h
    aautil.c
    aautil.h
)

target_link_libraries(aastr PUBLIC Allegro::Allegro)

add_library(AAStr::AAStr ALIAS aastr)
