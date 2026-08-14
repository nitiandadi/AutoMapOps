add_executable(automap_cli
    main.cpp
)

target_link_libraries(automap_cli
    PRIVATE
        AutoMapOps::map_io
        AutoMapOps::map_validation
        AutoMapOps::map_version
)

target_compile_features(automap_cli PRIVATE cxx_std_20)
target_compile_definitions(automap_cli PRIVATE
    AUTOMAP_PROJECT_VERSION="${PROJECT_VERSION}"
)

if(MSVC)
    target_compile_options(automap_cli PRIVATE /W4 /permissive- /utf-8)
else()
    target_compile_options(automap_cli PRIVATE -Wall -Wextra -Wpedantic)
endif()

set_target_properties(automap_cli PROPERTIES
    FOLDER "AutoMapOps/Apps"
)
