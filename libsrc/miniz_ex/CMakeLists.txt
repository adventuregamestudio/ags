add_library(miniz_ex)

set_target_properties(miniz_ex PROPERTIES
        C_STANDARD 11
        C_EXTENSIONS NO
        )
		
target_include_directories(miniz_ex PUBLIC . )

if (NOT AGS_USE_LOCAL_MINIZ)
    target_include_directories(miniz_ex PUBLIC ../miniz)
else()
    target_link_libraries(miniz_ex PUBLIC MiniZ::MiniZ)
endif()

target_sources(miniz_ex
        PRIVATE
        miniz_ex.c
        miniz_ex.h
        )

add_library(MiniZ_Ex::MiniZ_Ex ALIAS miniz_ex)
