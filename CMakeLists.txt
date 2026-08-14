cmake_minimum_required(VERSION 3.20)

project(
    AutoMapOps
    VERSION 0.1.0
    DESCRIPTION "Autonomous-driving map production and vehicle delivery learning platform"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(CTest)

message(STATUS "Configuring ${PROJECT_NAME} ${PROJECT_VERSION}")
message(STATUS "C++ standard: ${CMAKE_CXX_STANDARD}")

add_subdirectory(cpp)

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
