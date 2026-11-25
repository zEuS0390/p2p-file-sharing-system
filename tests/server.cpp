#include <iostream>
#include <unistd.h>
#include <csignal>
#include <string>

#include "p2p_fss/network/ConnectionManager.hpp"

// Main entry point of the program
int main(int argc, char* argv[])
{
  if (argc != 2)
  {
    std::cerr << "Usage: "
              << argv[0]
              << " <port>" 
    << std::endl;
    return 1;
  }

  static ConnectionManager server = ConnectionManager();

  auto handle_signal = []([[maybe_unused]] int signal_number)
  {
    server.setEventLoopStatus(false);
  };

  std::signal(SIGTERM, handle_signal);
  std::signal(SIGINT, handle_signal);

  server.initListeners(std::stoi(argv[1]));
  server.startListening();
  server.runEventLoop();
  server.stopListening();
  server.stopEventLoop();
}

