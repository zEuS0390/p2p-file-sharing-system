#include <iostream>

#include "core/network/Peer.hpp"

// Main entry point of the program
int main()
{
  Peer peer;

  peer.start();

  int file_descriptor {peer.connectToServer("localhost", 12345)};

  std::cin.get();

  // peer.sendMessage(file_descriptor, "Hello world");

  std::cin.get();

  peer.disconnectToServer(file_descriptor);

  peer.stop();

  return 0;
}
