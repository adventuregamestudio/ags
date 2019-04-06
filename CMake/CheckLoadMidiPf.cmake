include(CheckCXXSourceCompiles)
find_package(Allegro)

if(LINUX)
    # check_symbol_exists seems to have a problem with aliased aliases :)
    set(CMAKE_REQUIRED_LIBRARIES PkgConfig::ALLEGRO)
else()
    set(CMAKE_REQUIRED_LIBRARIES Allegro::Allegro)
endif()

set(CMAKE_REQUIRED_DEFINITIONS -DALLEGRO_NO_MAGIC_MAIN)

check_symbol_exists(load_midi_pf allegro.h HAVE_LOAD_MIDI_PF)
