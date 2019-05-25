add_library(cda)

set_target_properties(cda PROPERTIES
    C_STANDARD          11
    C_EXTENSIONS        NO
    LINKER_LANGUAGE     C       # need to specify this if macos (because no source)
)

target_include_directories(cda PUBLIC .)

target_sources(cda 
    PRIVATE
        libcda.h
        $<$<PLATFORM_ID:Windows>:
            windows.c
        >
        $<$<PLATFORM_ID:Linux>:
            linux.c
        >
)

if (WIN32)
    target_link_libraries(cda PUBLIC winmm)
endif()

add_library(Cda::Cda ALIAS cda)
