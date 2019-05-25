add_library(aldumb)

set_target_properties(aldumb PROPERTIES
    C_STANDARD 11
    C_EXTENSIONS NO
)

target_include_directories(aldumb PUBLIC include )

target_sources(aldumb 
    PRIVATE
    allegro/alplay.c
    allegro/datduh.c
    allegro/datit.c
    allegro/datmod.c
    allegro/dats3m.c
    allegro/datunld.c
    allegro/datxm.c
    allegro/packfile.c
    core/atexit.c
    core/duhlen.c
    core/dumbfile.c
    core/loadduh.c
    core/makeduh.c
    core/rawsig.c
    core/readduh.c
    core/register.c
    core/rendduh.c
    core/rendsig.c
    core/unload.c
    helpers/clickrem.c
    helpers/memfile.c
    helpers/resample.c
    helpers/sampbuf.c
    helpers/silence.c
    helpers/stdfile.c
    it/itload.c
    it/itmisc.c
    it/itorder.c
    it/itread.c
    it/itrender.c
    it/itunload.c
    it/loadmod.c
    it/loads3m.c
    it/loadxm.c
    it/readmod.c
    it/reads3m.c
    it/readxm.c
    it/xmeffect.c
)

target_link_libraries(aldumb PUBLIC Allegro::Allegro )

add_library(AlDumb::AlDumb ALIAS aldumb)
