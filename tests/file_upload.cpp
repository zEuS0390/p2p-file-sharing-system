#include <chrono>
#include <cstring>
#include <ios>
#include <iostream>
#include <ostream>
#include <string>
#include <thread>

#include "core/network/ConnectionManager.hpp"
#include "core/network/MessageHandler.hpp"
#include "core/types/MessageHeaders.hpp"

// Main Entry Point of the Program
int main(int argc, const char* argv[])
{
  if (argc != 4)
  {
    std::cerr << "Usage: "
              << argv[0]
              << " <hostname> <port> <filename>"
              << std::endl;
    return 1;
  }

  MessageHandler message_handler;
  ConnectionManager client {message_handler};

  int socket_descriptor {client.connect(argv[1], std::stoi(argv[2]))};

  std::thread eventThread {&ConnectionManager::runEventLoop, &client};

  std::string file_name {argv[3]};

  std::ifstream input_file_stream {file_name, std::ios::binary | std::ios::ate};
  if (!input_file_stream.is_open())
  {
    std::cerr << std::strerror(errno) << std::endl;
    input_file_stream.close();
    return 1;
  }
  const std::streamsize file_size {input_file_stream.tellg()};
    input_file_stream.close();

  FileUploadRequestHeader file_upload_request_header;
  file_upload_request_header.filename_size = file_name.size();
  file_upload_request_header.file_size = file_size;

  std::vector<char> bytes;
  bytes.resize(file_name.size() + sizeof(FileUploadRequestHeader));
  std::memcpy(bytes.data(), &file_upload_request_header, sizeof(FileUploadRequestHeader));
  std::memcpy(bytes.data() + sizeof(FileUploadRequestHeader), file_name.data(), file_name.size());

  int send_status {
    client.send(
        socket_descriptor,
        MessageType::FILE_UPLOAD_REQUEST,
        bytes.data(),
        bytes.size()
      )
  };

  if (send_status < 0)
  {
    std::cerr << strerror(errno) << std::endl;
    return 1;
  }

  std::cout << "Press enter to stop ...";
  std::cin.get();

  client.disconnect(socket_descriptor);

  client.stopEventLoop();

  eventThread.join();

  return 0;
}


