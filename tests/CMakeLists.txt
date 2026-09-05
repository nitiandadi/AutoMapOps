function(add_automap_test target source test_name)
    add_executable(${target} ${source})
    target_link_libraries(${target} PRIVATE ${ARGN})
    target_compile_features(${target} PRIVATE cxx_std_20)

    if(MSVC)
        target_compile_options(${target} PRIVATE /W4 /permissive- /utf-8)
    else()
        target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic)
    endif()

    add_test(NAME ${test_name} COMMAND ${target})
    set_target_properties(${target} PROPERTIES FOLDER "AutoMapOps/Tests")
endfunction()

# M1：最小工程架子
add_automap_test(
    automap_smoke_test
    m1/smoke_test.cpp
    automap.m1.smoke
    AutoMapOps::map_io
    AutoMapOps::map_validation
    AutoMapOps::map_version
)

# M2：Canonical 模型与物流园 V0
add_automap_test(
    automap_m2_model_test
    m2/model_test.cpp
    automap.m2.model
    AutoMapOps::map_core
)
add_automap_test(
    automap_m2_map_header_test
    m2/map_header_test.cpp
    automap.m2.map_header
    AutoMapOps::map_core
)
add_automap_test(
    automap_m2_road_lane_test
    m2/road_lane_test.cpp
    automap.m2.road_lane
    AutoMapOps::map_core
)
add_automap_test(
    automap_m2_lane_boundary_test
    m2/lane_boundary_test.cpp
    automap.m2.lane_boundary
    AutoMapOps::map_core
)
add_automap_test(
    automap_m2_junction_test
    m2/junction_test.cpp
    automap.m2.junction
    AutoMapOps::map_core
)
add_automap_test(
    automap_m2_scene_objects_test
    m2/scene_objects_test.cpp
    automap.m2.scene_objects
    AutoMapOps::map_core
)
add_automap_test(
    automap_m2_vehicle_profile_test
    m2/vehicle_profile_test.cpp
    automap.m2.vehicle_profile
    AutoMapOps::map_core
)
add_automap_test(
    automap_m2_map_data_test
    m2/map_data_test.cpp
    automap.m2.map_data
    AutoMapOps::map_core
)

# M3：Canonical IO、质检与 MapVersion V1
add_automap_test(
    automap_m3_canonical_json_test
    m3/canonical_json_test.cpp
    automap.m3.canonical_json
    AutoMapOps::map_io
)
target_compile_definitions(automap_m3_canonical_json_test PRIVATE
    AUTOMAP_SOURCE_DIR="${CMAKE_SOURCE_DIR}"
)

add_automap_test(
    automap_m3_unique_id_rule_test
    m3/unique_id_rule_test.cpp
    automap.m3.unique_id_rule
    AutoMapOps::map_validation
)

add_automap_test(
    automap_m3_reference_integrity_rule_test
    m3/reference_integrity_rule_test.cpp
    automap.m3.reference_integrity_rule
    AutoMapOps::map_validation
    AutoMapOps::map_io
)
target_compile_definitions(automap_m3_reference_integrity_rule_test PRIVATE
    AUTOMAP_SOURCE_DIR="${CMAKE_SOURCE_DIR}"
)

add_automap_test(
    automap_m3_basic_geometry_rule_test
    m3/basic_geometry_rule_test.cpp
    automap.m3.basic_geometry_rule
    AutoMapOps::map_validation
    AutoMapOps::map_io
)
target_compile_definitions(automap_m3_basic_geometry_rule_test PRIVATE
    AUTOMAP_SOURCE_DIR="${CMAKE_SOURCE_DIR}"
)

add_automap_test(
    automap_m3_topology_reciprocity_rule_test
    m3/topology_reciprocity_rule_test.cpp
    automap.m3.topology_reciprocity_rule
    AutoMapOps::map_validation
    AutoMapOps::map_io
)
target_compile_definitions(automap_m3_topology_reciprocity_rule_test PRIVATE
    AUTOMAP_SOURCE_DIR="${CMAKE_SOURCE_DIR}"
)

add_automap_test(
    automap_m3_connection_geometry_rule_test
    m3/connection_geometry_rule_test.cpp
    automap.m3.connection_geometry_rule
    AutoMapOps::map_validation
    AutoMapOps::map_io
)
target_compile_definitions(automap_m3_connection_geometry_rule_test PRIVATE
    AUTOMAP_SOURCE_DIR="${CMAKE_SOURCE_DIR}"
)

add_automap_test(
    automap_m3_network_reachability_rule_test
    m3/network_reachability_rule_test.cpp
    automap.m3.network_reachability_rule
    AutoMapOps::map_validation
    AutoMapOps::map_io
)
target_compile_definitions(automap_m3_network_reachability_rule_test PRIVATE
    AUTOMAP_SOURCE_DIR="${CMAKE_SOURCE_DIR}"
)

add_test(
    NAME automap.m3.inspect
    COMMAND automap_cli inspect "${CMAKE_SOURCE_DIR}/maps/drafts/logistics_park_v0.json"
)

# CV：Canonical 1.1 曲线与 WebGL 可视化改造
add_automap_test(
    automap_cv_curve_geometry_test
    cv/curve_geometry_test.cpp
    automap.cv.curve_geometry
    AutoMapOps::map_core
)
add_automap_test(
    automap_cv_canonical_curve_json_test
    cv/canonical_curve_json_test.cpp
    automap.cv.canonical_curve_json
    AutoMapOps::map_io
)
target_compile_definitions(automap_cv_canonical_curve_json_test PRIVATE
    AUTOMAP_SOURCE_DIR="${CMAKE_SOURCE_DIR}"
)
add_automap_test(
    automap_cv_curve_validation_test
    cv/curve_validation_test.cpp
    automap.cv.curve_validation
    AutoMapOps::map_validation
)
