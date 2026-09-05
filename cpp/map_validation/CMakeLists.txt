add_library(automap_map_validation STATIC
    include/automap/validation/basic_geometry_rule.hpp
    include/automap/validation/connection_geometry_rule.hpp
    include/automap/validation/network_reachability_rule.hpp
    include/automap/validation/map_validation.hpp
    include/automap/validation/reference_integrity_rule.hpp
    include/automap/validation/topology_reciprocity_rule.hpp
    include/automap/validation/validation_issue.hpp
    include/automap/validation/validation_report.hpp
    include/automap/validation/validation_rule.hpp
    include/automap/validation/unique_id_rule.hpp
    src/basic_geometry_rule.cpp
    src/connection_geometry_rule.cpp
    src/network_reachability_rule.cpp
    src/reference_integrity_rule.cpp
    src/topology_reciprocity_rule.cpp
    src/unique_id_rule.cpp
    src/validation_report.cpp
)

add_library(AutoMapOps::map_validation ALIAS automap_map_validation)

target_include_directories(automap_map_validation
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_link_libraries(automap_map_validation
    PUBLIC
        AutoMapOps::map_core
)

target_compile_features(automap_map_validation PUBLIC cxx_std_20)

if(MSVC)
    target_compile_options(automap_map_validation PRIVATE /W4 /permissive- /utf-8)
else()
    target_compile_options(automap_map_validation PRIVATE -Wall -Wextra -Wpedantic)
endif()

set_target_properties(automap_map_validation PROPERTIES
    OUTPUT_NAME automap_map_validation
    FOLDER "AutoMapOps/Validation"
)
