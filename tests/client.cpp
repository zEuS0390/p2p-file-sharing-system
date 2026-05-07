#include <core/network/Client.hpp>
#include <cstdlib>
#include <iostream>
#include <thread>
#include <chrono>

// Main entry point of the program
int main(int argc, char* argv[])
{
  if (argc == 1)
  {
    std::cerr << argv[0] << " requires two arguments (hostname, port)." << std::endl;
    return 1;
  }

  Client client;
  int server_socket_descriptor = client.connectToServer(argv[1], atoi(argv[2]));

  if (server_socket_descriptor < 0)
    return 1;

  std::this_thread::sleep_for(std::chrono::seconds(10));

  client.disconnectToServer(server_socket_descriptor);

  return 0;
}
