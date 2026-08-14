add_library(automap_map_version STATIC
    include/automap/version/map_version.hpp
    include/automap/version/version_id.hpp
    include/automap/version/version_publisher.hpp
    src/version_id.cpp
    src/version_publisher.cpp
)

add_library(AutoMapOps::map_version ALIAS automap_map_version)

target_include_directories(automap_map_version
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_link_libraries(automap_map_version
    PUBLIC
        AutoMapOps::map_core
        AutoMapOps::map_validation
)

target_compile_features(automap_map_version PUBLIC cxx_std_20)

if(MSVC)
    target_compile_options(automap_map_version PRIVATE /W4 /permissive- /utf-8)
else()
    target_compile_options(automap_map_version PRIVATE -Wall -Wextra -Wpedantic)
endif()

set_target_properties(automap_map_version PROPERTIES
    OUTPUT_NAME automap_map_version
    FOLDER "AutoMapOps/Version"
)
