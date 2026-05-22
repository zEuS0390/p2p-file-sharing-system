#include "core/network/Server.hpp"
#include <unistd.h>
#include <csignal>
#include <thread>

volatile sig_atomic_t stop {0};

void handler(int s)
{
  stop = 1;
}

// Main entry point of the program
int main()
{
  struct sigaction sigIntHandler;

  sigIntHandler.sa_handler = handler;
  sigemptyset(&sigIntHandler.sa_mask);
  sigIntHandler.sa_flags = 0;

  sigaction(SIGINT, &sigIntHandler, nullptr);

  Server server;
  std::thread listeningThread {&Server::startListening, std::ref(server)};
  std::thread monitoringThread (&Server::runEventLoop, std::ref(server));

  while (!stop)
  {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }

  server.stopListening();
  server.stopEventLoop();

  listeningThread.join();
  monitoringThread.join();

  return 0;
}
