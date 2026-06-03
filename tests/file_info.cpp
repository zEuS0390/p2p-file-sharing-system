#include "core/network/ClientMessageHandler.hpp"
#include "core/types/MessageHeaders.hpp"
#include "core/types/MessageType.hpp"
#include "core/network/Client.hpp"
#include <iostream>
#include <cstdlib>
#include <cstring>

// Main entry point of the program
int main(int argc, char* argv[])
{
  if (argc != 3)
  {
    std::cerr << "Usage: " << argv[0] << " <hostname> <port>" << std::endl;
    return 1;
  }

  ClientMessageHandler client_message_handler;
  Client client {client_message_handler};
  int server_socket_descriptor = client.connectToServer(argv[1], atoi(argv[2]));

  if (server_socket_descriptor < 0)
  {
    std::cout << "Error connecting to the server." << std::endl;
    return 1;
  }

  client.start();

  std::cout << "Press enter to continue..." << std::endl;
  std::cin.get();

  MessageHeader message_header;
  message_header.type = MessageType::FILE_INFO;
  message_header.payload_size = 1;

  ssize_t send_status;

  send_status = client.sendAll(
    server_socket_descriptor,
    MessageType::FILE_INFO,
    reinterpret_cast<char*>(&message_header),
    sizeof(message_header)
  );

  if (send_status < 0)
  {
    std::cerr << strerror(errno) << std::endl;
    return 1;
  }

  std::cout << "Press enter to continue..." << std::endl;
  std::cin.get();

  client.disconnectToServer(server_socket_descriptor);

  client.stop();

  return 0;
}
