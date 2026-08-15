add_library(automap_map_core STATIC
    include/automap/core/geometry.hpp
    include/automap/core/junction.hpp
    include/automap/core/lane_boundary.hpp
    include/automap/core/map_data.hpp
    include/automap/core/map_core.hpp
    include/automap/core/map_header.hpp
    include/automap/core/map_id.hpp
    include/automap/core/map_objects.hpp
    include/automap/core/object_id.hpp
    include/automap/core/road_lane.hpp
    include/automap/core/scene_objects.hpp
    include/automap/core/vehicle_profile.hpp
    src/geometry.cpp
    src/junction.cpp
    src/lane_boundary.cpp
    src/map_data.cpp
    src/map_header.cpp
    src/map_id.cpp
    src/road_lane.cpp
    src/scene_objects.cpp
    src/vehicle_profile.cpp
)

add_library(AutoMapOps::map_core ALIAS automap_map_core)

target_include_directories(automap_map_core
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_compile_features(automap_map_core PUBLIC cxx_std_20)

if(MSVC)
    target_compile_options(automap_map_core PRIVATE /W4 /permissive- /utf-8)
else()
    target_compile_options(automap_map_core PRIVATE -Wall -Wextra -Wpedantic)
endif()

set_target_properties(automap_map_core PROPERTIES
    OUTPUT_NAME automap_map_core
    FOLDER "AutoMapOps/Core"
)
