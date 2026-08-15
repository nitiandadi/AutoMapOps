add_executable(automap_smoke_test
    smoke_test.cpp
)

target_link_libraries(automap_smoke_test
    PRIVATE
        AutoMapOps::map_io
        AutoMapOps::map_validation
        AutoMapOps::map_version
)

target_compile_features(automap_smoke_test PRIVATE cxx_std_20)

if(MSVC)
    target_compile_options(automap_smoke_test PRIVATE /W4 /permissive- /utf-8)
else()
    target_compile_options(automap_smoke_test PRIVATE -Wall -Wextra -Wpedantic)
endif()

add_test(
    NAME automap.m1.smoke
    COMMAND automap_smoke_test
)

set_target_properties(automap_smoke_test PROPERTIES
    FOLDER "AutoMapOps/Tests"
)

add_executable(automap_m2_model_test
    m2_model_test.cpp
)

target_link_libraries(automap_m2_model_test
    PRIVATE
        AutoMapOps::map_core
)

target_compile_features(automap_m2_model_test PRIVATE cxx_std_20)

if(MSVC)
    target_compile_options(automap_m2_model_test PRIVATE /W4 /permissive- /utf-8)
else()
    target_compile_options(automap_m2_model_test PRIVATE -Wall -Wextra -Wpedantic)
endif()

add_test(
    NAME automap.m2.model
    COMMAND automap_m2_model_test
)

set_target_properties(automap_m2_model_test PROPERTIES
    FOLDER "AutoMapOps/Tests"
)

add_executable(automap_m2_map_header_test
    m2_map_header_test.cpp
)

target_link_libraries(automap_m2_map_header_test
    PRIVATE
        AutoMapOps::map_core
)

target_compile_features(automap_m2_map_header_test PRIVATE cxx_std_20)

if(MSVC)
    target_compile_options(automap_m2_map_header_test PRIVATE /W4 /permissive- /utf-8)
else()
    target_compile_options(automap_m2_map_header_test PRIVATE -Wall -Wextra -Wpedantic)
endif()

add_test(
    NAME automap.m2.map_header
    COMMAND automap_m2_map_header_test
)

set_target_properties(automap_m2_map_header_test PROPERTIES
    FOLDER "AutoMapOps/Tests"
)

add_executable(automap_m2_road_lane_test
    m2_road_lane_test.cpp
)

target_link_libraries(automap_m2_road_lane_test
    PRIVATE
        AutoMapOps::map_core
)

target_compile_features(automap_m2_road_lane_test PRIVATE cxx_std_20)

if(MSVC)
    target_compile_options(automap_m2_road_lane_test PRIVATE /W4 /permissive- /utf-8)
else()
    target_compile_options(automap_m2_road_lane_test PRIVATE -Wall -Wextra -Wpedantic)
endif()

add_test(
    NAME automap.m2.road_lane
    COMMAND automap_m2_road_lane_test
)

set_target_properties(automap_m2_road_lane_test PROPERTIES
    FOLDER "AutoMapOps/Tests"
)

add_executable(automap_m2_lane_boundary_test
    m2_lane_boundary_test.cpp
)

target_link_libraries(automap_m2_lane_boundary_test
    PRIVATE
        AutoMapOps::map_core
)

target_compile_features(automap_m2_lane_boundary_test PRIVATE cxx_std_20)

if(MSVC)
    target_compile_options(automap_m2_lane_boundary_test PRIVATE /W4 /permissive- /utf-8)
else()
    target_compile_options(automap_m2_lane_boundary_test PRIVATE -Wall -Wextra -Wpedantic)
endif()

add_test(
    NAME automap.m2.lane_boundary
    COMMAND automap_m2_lane_boundary_test
)

set_target_properties(automap_m2_lane_boundary_test PROPERTIES
    FOLDER "AutoMapOps/Tests"
)

function(add_automap_map_core_test target source test_name)
    add_executable(${target} ${source})
    target_link_libraries(${target} PRIVATE AutoMapOps::map_core)
    target_compile_features(${target} PRIVATE cxx_std_20)
    if(MSVC)
        target_compile_options(${target} PRIVATE /W4 /permissive- /utf-8)
    else()
        target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic)
    endif()
    add_test(NAME ${test_name} COMMAND ${target})
    set_target_properties(${target} PROPERTIES FOLDER "AutoMapOps/Tests")
endfunction()

add_automap_map_core_test(
    automap_m2_junction_test
    m2_junction_test.cpp
    automap.m2.junction
)
add_automap_map_core_test(
    automap_m2_scene_objects_test
    m2_scene_objects_test.cpp
    automap.m2.scene_objects
)
add_automap_map_core_test(
    automap_m2_vehicle_profile_test
    m2_vehicle_profile_test.cpp
    automap.m2.vehicle_profile
)
add_automap_map_core_test(
    automap_m2_map_data_test
    m2_map_data_test.cpp
    automap.m2.map_data
)
