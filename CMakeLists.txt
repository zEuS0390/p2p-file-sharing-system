# Set minimum required CMake version
cmake_minimum_required(VERSION 3.17)

# Set project name and version
project(p2p_network_system VERSION 1.0.0)

# Set C++ compilation flags
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/03597a01ee50ed33e9dfd640b249b4be3799d395.zip
  DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
FetchContent_MakeAvailable(googletest)

# Set the full version name of the project
set(PROJECT_VERSION_FULL "${PROJECT_VERSION}-beta")

# Set the output directory for the binaries
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")

set(BUILD_SHARED_LIBS ON)

set(
  NETWORK_SRC_FILES
  "src/ConnectionManager.cpp"
  "src/MessageHandler.cpp"
  "src/IOMultiplexer.cpp"
  "src/ThreadPool.cpp"
)

add_library(
  core
  SHARED
  ${NETWORK_SRC_FILES}
)

target_include_directories(
  core
  PUBLIC
  "${CMAKE_CURRENT_SOURCE_DIR}/include"
)

set_target_properties(
  core
  PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY
  "${CMAKE_BINARY_DIR}/lib"
)

# add_subdirectory("${CMAKE_SOURCE_DIR}/lib/core")
add_subdirectory("${CMAKE_SOURCE_DIR}/tests")
add_subdirectory("${CMAKE_SOURCE_DIR}/gtests")

