# Set minimum required CMake version
cmake_minimum_required(VERSION 3.15)

# Set project name and version
project(p2p_network_system VERSION 1.0.0)

# Set C++ compilation flags
set(CMAKE_CXX_FLAGS "-fsanitize=address -Wall -Wextra -g")

# Set the full version name of the project
set(PROJECT_VERSION_FULL "${PROJECT_VERSION}-beta")

# Set the output directory for the binaries
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")

set(BUILD_SHARED_LIBS ON)

add_subdirectory("${CMAKE_SOURCE_DIR}/lib/core")
add_subdirectory("${CMAKE_SOURCE_DIR}/tests")

