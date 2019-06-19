add_library(hq2x)

set_target_properties(hq2x PROPERTIES
    CXX_STANDARD 14
    CXX_EXTENSIONS NO
)

target_include_directories(hq2x PUBLIC . )

target_sources(hq2x 
    PRIVATE
    hq2x3x.cpp
)

target_link_libraries(hq2x PUBLIC AGS::Common)

add_library(HQ2X::hq2x ALIAS hq2x)
