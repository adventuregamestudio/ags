add_library(glad)

set_target_properties(glad PROPERTIES
    C_STANDARD          11
    C_EXTENSIONS        NO
)

target_include_directories(glad PUBLIC include)

target_sources(glad 
    PRIVATE
        include/KHR/khrplatform.h
        include/glad/glad.h
        src/glad.c
)

target_link_libraries(glad PUBLIC ${CMAKE_DL_LIBS})
if (WIN32)
    target_link_libraries(glad PUBLIC opengl32)
endif()

add_library(Glad::Glad ALIAS glad)
