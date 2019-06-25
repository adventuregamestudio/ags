add_library(cda)

set_target_properties(cda PROPERTIES
    C_STANDARD          11
    C_EXTENSIONS        NO
    LINKER_LANGUAGE     C
)

target_include_directories(cda PUBLIC .)

target_sources(cda 
    PRIVATE
        libcda.h
)

if(WIN32)
    target_sources(cda PRIVATE 
        windows.c
    )
endif()

if(LINUX)
    target_sources(cda PRIVATE 
        linux.c
    )
endif()


if (WIN32)
    target_link_libraries(cda PUBLIC winmm)
endif()

add_library(Cda::Cda ALIAS cda)
