cmake_minimum_required(VERSION 3.10)
project(SharedMemoryJSON VERSION 1.0)

# Set C++17 standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add threading support
find_package(Threads REQUIRED)

# Include FetchContent for nlohmann_json
include(FetchContent)

# Fetch nlohmann_json
FetchContent_Declare(
    nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.11.3
)
FetchContent_MakeAvailable(nlohmann_json)

# Header-only library interface
add_library(shared_memory_json INTERFACE)
target_include_directories(shared_memory_json INTERFACE 
    ${CMAKE_CURRENT_SOURCE_DIR}
)
target_link_libraries(shared_memory_json INTERFACE 
    nlohmann_json::nlohmann_json
    Threads::Threads
)

# Add platform-specific libraries
if(UNIX AND NOT APPLE)
    target_link_libraries(shared_memory_json INTERFACE rt pthread)
endif()

# Examples
add_executable(writer example_writer.cpp)
target_link_libraries(writer PRIVATE shared_memory_json)

add_executable(reader example_reader.cpp)
target_link_libraries(reader PRIVATE shared_memory_json)

add_executable(simple_reader example_simple_reader.cpp)
target_link_libraries(simple_reader PRIVATE shared_memory_json)

# Advanced examples
add_executable(service example_service.cpp)
target_link_libraries(service PRIVATE shared_memory_json)

add_executable(controller example_controller.cpp)
target_link_libraries(controller PRIVATE shared_memory_json)

add_executable(monitor example_monitor.cpp)
target_link_libraries(monitor PRIVATE shared_memory_json)

# Test suite
add_executable(test_suite test_suite.cpp)
target_link_libraries(test_suite PRIVATE shared_memory_json)

# Installation
install(FILES shared_memory_json.hpp 
    DESTINATION include/shared_memory
)

# Enable warnings
if(MSVC)
    target_compile_options(shared_memory_json INTERFACE /W4)
else()
    target_compile_options(shared_memory_json INTERFACE -Wall -Wextra -pedantic)
endif()
