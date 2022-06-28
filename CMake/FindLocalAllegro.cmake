add_subdirectory(libsrc/allegro EXCLUDE_FROM_ALL)
target_compile_definitions (allegro
INTERFACE 
     ALLEGRO_STATICLINK
)
add_library(Allegro::Allegro ALIAS allegro)
