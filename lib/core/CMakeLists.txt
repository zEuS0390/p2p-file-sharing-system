cmake_minimum_required(VERSION 3.15)

set(NETWORK_SRC_FILES
    "src/Peer.cpp"
    "src/Client.cpp"
    "src/Server.cpp"
    "src/ConnectionManager.cpp"
    "src/ClientMessageHandler.cpp"
    "src/ServerMessageHandler.cpp"
    "src/ServerConnectionManager.cpp"
    "src/ClientConnectionManager.cpp")

add_library(core SHARED ${NETWORK_SRC_FILES})

target_include_directories(core PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")

set_target_properties(core PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")

