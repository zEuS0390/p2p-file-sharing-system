#include <iostream>
#include <string>
#include <unistd.h>
#include <csignal>

#include "core/network/MessageHandler.hpp"
#include "core/network/ConnectionManager.hpp"

static ConnectionManager* server {nullptr};

volatile std::sig_atomic_t shutdown_requested {0};

void handle_signal(int signal_number)
{
  server->m_is_event_running = 0;
}

// Main entry point of the program
int main(int argc, char* argv[])
{
  if (argc != 2)
  {
    std::cerr << "Usage: " << argv[0] << " <port>" << std::endl;
    return 1;
  }

  std::signal(SIGTERM, handle_signal);
  std::signal(SIGINT, handle_signal);

  MessageHandler message_handler;
  server = new ConnectionManager{message_handler};
  server->initListeners(std::stoi(argv[1]));
  server->startListening();
  server->runEventLoop();
  server->stopListening();
  server->stopEventLoop();
  delete server;
  return 0;
}

