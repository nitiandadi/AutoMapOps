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
