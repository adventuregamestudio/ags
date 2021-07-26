add_library(tinyxml2)

set_target_properties(tinyxml2 PROPERTIES
        CXX_STANDARD 11
        CXX_EXTENSIONS NO
        )

target_include_directories(tinyxml2 PUBLIC . )

target_sources(tinyxml2
        PRIVATE
        tinyxml2.cpp
        tinyxml2.h
        )

add_library(TinyXML2::TinyXML2 ALIAS tinyxml2)