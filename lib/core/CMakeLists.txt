cmake_minimum_required(VERSION 3.15)

set(CORE_FILES
    "network/Client.cpp"
    "network/Server.cpp"
    "network/Socket.cpp")

add_library(core SHARED ${CORE_FILES})

target_include_directories(core PUBLIC "${CMAKE_SOURCE_DIR}/include")

set_target_properties(core PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")

