cmake_minimum_required(VERSION 3.15)

add_executable(server "${CMAKE_SOURCE_DIR}/tests/server.cpp")
add_executable(client "${CMAKE_SOURCE_DIR}/tests/client.cpp")

set_target_properties(server PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/tests")
set_target_properties(client PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/tests")

target_link_libraries(server PRIVATE core)
target_link_libraries(client PRIVATE core)

