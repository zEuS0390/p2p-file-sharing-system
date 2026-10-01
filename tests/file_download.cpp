#include <cstring>
#include <iostream>
#include <string>

#include "core/network/Client.hpp"
#include "core/network/ClientMessageHandler.hpp"
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

  ClientMessageHandler client_message_handler;
  Client client {client_message_handler};

  int socket_descriptor {client.connect(argv[1], std::stoi(argv[2]))};

  client.start();

  std::string file_name {argv[3]};
  FileRequestHeader file_request_header(file_name.size());

  std::vector<char> bytes {};
  bytes.reserve(file_name.size() + sizeof(FileRequestHeader));
  bytes.resize(file_name.size() + sizeof(FileRequestHeader));
  std::memcpy(bytes.data(), &file_request_header, sizeof(FileRequestHeader));
  std::memcpy(bytes.data() + sizeof(FileRequestHeader), file_name.data(), file_name.size());

  int send_status {
    client.send(
        socket_descriptor,
        MessageType::FILE_DOWNLOAD_REQUEST,
        bytes.data(),
        bytes.size()
      )
  };

  if (send_status < 0)
  {
    std::cerr << strerror(errno) << std::endl;
    return 1;
  }

  std::cout << "Press enter to continue..." << std::endl;
  std::cin.get();

  client.disconnect(socket_descriptor);

  client.stop();

  return 0;
}
