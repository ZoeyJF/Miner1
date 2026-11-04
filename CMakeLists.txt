cmake_minimum_required(VERSION 3.16)
project(MidnightMobileClient VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Compiler flags for optimization and warnings
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-Wall -Wextra -Wpedantic -O2)
endif()

# Include directories
include_directories(include)

# Core library
add_library(midnight_mobile_client STATIC
    src/midnight_mobile_client.cpp
)

# CLI Executable
add_executable(midnight_cli
    src/main.cpp
)
target_link_libraries(midnight_cli PRIVATE midnight_mobile_client)

# Enable testing
enable_testing()
add_executable(test_client
    tests/test_client.cpp
)
target_link_libraries(test_client PRIVATE midnight_mobile_client)
add_test(NAME TestMidnightClient COMMAND test_client)
