add_library(automap_map_io STATIC
    include/automap/io/canonical_json.hpp
    include/automap/io/map_io.hpp
    src/canonical_json.cpp
)

add_library(AutoMapOps::map_io ALIAS automap_map_io)

target_include_directories(automap_map_io
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_link_libraries(automap_map_io
    PUBLIC
        AutoMapOps::map_core
)

target_compile_features(automap_map_io PUBLIC cxx_std_20)

if(MSVC)
    target_compile_options(automap_map_io PRIVATE /W4 /permissive- /utf-8)
else()
    target_compile_options(automap_map_io PRIVATE -Wall -Wextra -Wpedantic)
endif()

set_target_properties(automap_map_io PROPERTIES
    OUTPUT_NAME automap_map_io
    FOLDER "AutoMapOps/IO"
)
