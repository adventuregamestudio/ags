add_library(miniz)

set_target_properties(miniz PROPERTIES
        C_STANDARD 11
        C_EXTENSIONS NO
        )

target_include_directories(miniz PUBLIC . )

target_sources(miniz
        PRIVATE
        miniz.c
        miniz.h
        )

add_library(MiniZ::MiniZ ALIAS miniz)