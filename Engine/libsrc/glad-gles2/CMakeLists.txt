add_library(glad-gles2)

set_target_properties(glad-gles2 PROPERTIES
    C_STANDARD          11
    C_EXTENSIONS        NO
)

target_include_directories(glad-gles2 PUBLIC include)

target_sources(glad-gles2 
    PRIVATE
        include/KHR/khrplatform.h
        include/glad/glad.h
        src/glad.c
)

target_link_libraries(glad-gles2 PUBLIC ${CMAKE_DL_LIBS})

add_library(Glad::GladGLES2 ALIAS glad-gles2)
