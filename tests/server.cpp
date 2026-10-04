#include <iostream>
#include <string>
#include <thread>
#include <unistd.h>
#include <csignal>

#include "core/network/MessageHandler.hpp"
#include "core/network/ConnectionManager.hpp"

// Main entry point of the program
int main(int argc, char* argv[])
{
  if (argc != 2)
  {
    std::cerr << "Usage: " << argv[0] << " <port>" << std::endl;
    return 1;
  }

  MessageHandler message_handler;
  ConnectionManager server {message_handler};

  std::thread eventThread {&ConnectionManager::runEventLoop, &server};
  std::cout << "Starting listener on port " << argv[1] << std::endl;
  server.initListeners(std::stoi(argv[1]));
  server.startListening(); // EXAMPLE SOCKET ID 5

  std::cout << "Press any key to pause the listener ..." << std::endl;
  std::cin.get();
  std::cout << "Pausing listener." << std::endl;

  server.stopListening(); // EXAMPLE SOCKET ID 5 DISABLES I/O COMMUNICATION BUT NOT CLOSED
  std::cout << "Listener paused." << std::endl;

  std::cout << "Press any key to resume the listener ..." << std::endl;
  std::cin.get();
  std::cout << "Resuming listener." << std::endl;

  server.startListening(); // EXAMPLE SOCKET ID: 9
  std::cout << "Listener resumed." << std::endl;

  std::cout << "Press any key to stop the server ..." << std::endl;
  std::cin.get();

  server.stopListening(); // EXAMPLE SOCKET ID 9 DISABLES I/O COMMUNICATION BUT NOT CLOSED
  server.stopEventLoop();

  eventThread.join();

  return 0;
}

