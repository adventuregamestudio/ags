add_subdirectory(libsrc/allegro EXCLUDE_FROM_ALL)
target_compile_definitions (allegro
INTERFACE 
     ALLEGRO_STATICLINK
     ALLEGRO_NO_COMPATIBILITY 
     ALLEGRO_NO_FIX_ALIASES 
     ALLEGRO_NO_FIX_CLASS
     ALLEGRO_NO_KEY_DEFINES
)
add_library(Allegro::Allegro ALIAS allegro)
