add_library(mojoal)

set_target_properties(mojoal PROPERTIES
        C_STANDARD 11
        C_EXTENSIONS NO
        )

target_include_directories(mojoal PUBLIC . )

target_sources(mojoal
        PRIVATE
        mojoal.c
        AL/al.h
        AL/alc.h
        )

if(NOT EMSCRIPTEN)
    target_link_libraries(mojoal PUBLIC SDL2::SDL2)
else()
    target_link_libraries(mojoal PUBLIC ${SDL2_LIBRARIES})
endif()


target_compile_definitions(mojoal PUBLIC AL_LIBTYPE_STATIC)

add_library(MojoAL::MojoAL ALIAS mojoal)