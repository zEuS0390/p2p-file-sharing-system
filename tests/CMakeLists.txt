cmake_minimum_required(VERSION 3.17)

set(
  CPP_SOURCES
  "server.cpp"
  "client.cpp"
  "peer.cpp"
  "file_download.cpp"
  "file_upload.cpp"
  "file_info.cpp"
  "multiple_file_downloads.cpp"
  "multiple_file_uploads.cpp"
  "use_thread_pool.cpp"
)

foreach(src IN LISTS CPP_SOURCES)

  # Extract the filename without its directory path or file extension
  get_filename_component(target_name "${src}" NAME_WE)

  # Remove all underscores from the filename to generate a clean target name
  string(REPLACE "_" "" target_name "${target_name}")

  # Create an executable target using the generated target name and the corresponding source file
  add_executable(
    ${target_name}
    "${CMAKE_CURRENT_SOURCE_DIR}/${src}"
  )

  # Set the output directory for all generated executables to the tests folder
  set_target_properties(
    ${target_name}
    PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY
    "${CMAKE_BINARY_DIR}/tests"
  )

  # Link the executable against the p2p_fss library
  target_link_libraries(${target_name} PRIVATE p2p_fss)

endforeach()

