#include <iostream>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <thread>
#include <chrono>
#include <ios>

#include "core/network/ClientMessageHandler.hpp"
#include "core/types/MessageType.hpp"
#include "core/network/Client.hpp"

// Main entry point of the program
int main(int argc, char* argv[])
{
  if (argc != 5)
  {
    std::cerr << "Usage: "
              << argv[0]
              << " <hostname> <port> <speed_milliseconds> <file_path>"
              << std::endl;
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

  std::fstream file{argv[4], std::ios::in};

  if (!(file.is_open()))
  {
    std::cerr << "Error opening the file." << std::endl;
    return 1;
  }

  char ch;
  while (file.get(ch))
  {
    ssize_t send_status;
    send_status = client.send(
      server_socket_descriptor,
      MessageType::MESSAGE,
      std::string(1, ch).c_str(),
      1
    );
    if (send_status < 0)
      std::cout << "Error sending the message to server." << std::endl;
    std::this_thread::sleep_for(std::chrono::milliseconds(std::stoi(argv[3])));
  }

  std::cout << "Press enter to continue..." << std::endl;
  std::cin.get();

  client.disconnect(server_socket_descriptor);

  client.stop();

  return 0;
}
