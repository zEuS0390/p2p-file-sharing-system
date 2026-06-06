#include <iostream>
#include <unistd.h>
#include <csignal>

#include "core/network/ServerMessageHandler.hpp"
#include "core/network/Server.hpp"

// Main entry point of the program
int main(int argc, char* argv[])
{
  if (argc != 2)
  {
    std::cerr << "Usage: " << argv[0] << " <port>" << std::endl;
    return 1;
  }

  ServerMessageHandler server_message_handler;
  Server server {server_message_handler};

  server.start(std::stoi(argv[1]));

  std::cout << "Press any key to stop..." << std::endl;
  std::cin.get();

  server.stop();

  return 0;
}
