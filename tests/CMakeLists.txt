cmake_minimum_required(VERSION 3.15)

add_executable(peer "${CMAKE_CURRENT_SOURCE_DIR}/peer.cpp")
add_executable(server "${CMAKE_CURRENT_SOURCE_DIR}/server.cpp")
add_executable(client "${CMAKE_CURRENT_SOURCE_DIR}/client.cpp")

set_target_properties(peer PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/tests")
set_target_properties(server PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/tests")
set_target_properties(client PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/tests")

target_link_libraries(peer PRIVATE core)
target_link_libraries(server PRIVATE core)
target_link_libraries(client PRIVATE core)

