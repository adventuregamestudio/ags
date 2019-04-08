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
        $<$<PLATFORM_ID:Windows>:
            include/glad/glad_wgl.h
            src/glad_wgl.c
        >
        $<$<PLATFORM_ID:Linux>:
            include/glad/glad_glx.h
            src/glad_glx.c
        >
)

target_link_libraries(glad PUBLIC ${CMAKE_DL_LIBS})
if (WIN32)
    target_link_libraries(glad PUBLIC opengl32)
endif()

add_library(Glad::Glad ALIAS glad)
