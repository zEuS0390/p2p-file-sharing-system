#include <iostream>
#include <cstdlib>
#include <cstring>

#include "core/network/ClientMessageHandler.hpp"
#include "core/types/MessageHeaders.hpp"
#include "core/types/MessageType.hpp"
#include "core/network/Client.hpp"

// Main entry point of the program
int main(int argc, char* argv[])
{
  if (argc != 4)
  {
    std::cerr << "Usage: " << argv[0] << " <hostname> <port> <filename>" << std::endl;
    return 1;
  }

  ClientMessageHandler client_message_handler;
  Client client {client_message_handler};
  int server_socket_descriptor = client.connect(argv[1], atoi(argv[2]));

  if (server_socket_descriptor < 0)
  {
    std::cout << "Error connecting to the server." << std::endl;
    return 1;
  }

  client.start();

  std::cout << "Press enter to continue..." << std::endl;
  std::cin.get();

  std::string file_name {argv[3]};

  FileRequestHeader file_request_header;
  file_request_header.filename_size = file_name.size();

  std::vector<char> payload;
  payload.resize(sizeof(FileRequestHeader) + file_name.size());

  std::memcpy(payload.data(), &file_request_header, sizeof(FileRequestHeader));
  std::memcpy(payload.data() + sizeof(FileRequestHeader), file_name.data(), file_name.size());

  ssize_t send_status;

  send_status = client.send(
    server_socket_descriptor,
    MessageType::FILE_REQUEST,
    payload.data(),
    payload.size()
  );

  if (send_status < 0)
  {
    std::cerr << strerror(errno) << std::endl;
    return 1;
  }

  std::cout << "Press enter to continue..." << std::endl;
  std::cin.get();

  client.disconnect(server_socket_descriptor);

  client.stop();

  return 0;
}
