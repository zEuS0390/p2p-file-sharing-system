#include <iostream>
#include <cstdlib>
#include <cstring>
#include <thread>

#include "core/network/MessageHandler.hpp"
#include "core/network/ConnectionManager.hpp"
#include "core/types/MessageHeaders.hpp"
#include "core/types/MessageType.hpp"

// Main entry point of the program
int main(int argc, char* argv[])
{
  if (argc != 4)
  {
    std::cerr << "Usage: " << argv[0] << " <hostname> <port> <filename>" << std::endl;
    return 1;
  }

  MessageHandler message_handler;
  ConnectionManager client {message_handler};
  int server_socket_descriptor = client.connect(argv[1], atoi(argv[2]));

  if (server_socket_descriptor < 0)
  {
    std::cout << "Error connecting to the server." << std::endl;
    return 1;
  }

  std::thread eventThread {&ConnectionManager::runEventLoop, &client};

  std::string file_name {argv[3]};

  FileRequestHeader file_request_header;
  file_request_header.filename_size = file_name.size();

  std::vector<char> payload;
  payload.reserve(sizeof(FileRequestHeader) + file_name.size());
  payload.resize(sizeof(FileRequestHeader) + file_name.size());
  std::memcpy(payload.data(), &file_request_header, sizeof(FileRequestHeader));
  std::memcpy(payload.data() + sizeof(FileRequestHeader), file_name.data(), file_name.size());

  int send_status;

  send_status = client.send(
    server_socket_descriptor,
    MessageType::FILE_INFO_REQUEST,
    payload.data(),
    payload.size()
  );

  if (send_status < 0)
  {
    std::cerr << strerror(errno) << std::endl;
    return 1;
  }

  std::cin.get();

  client.disconnect(server_socket_descriptor);

  client.stopEventLoop();

  eventThread.join();

  return 0;
}
