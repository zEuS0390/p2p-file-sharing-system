#include <iostream>
#include <fstream>
#include <string>
#include <atomic>
#include <thread>

#include "core/network/MessageHandler.hpp"
#include "core/network/ConnectionManager.hpp"

std::atomic<bool> is_running {true};
std::atomic<int> socket_descriptor {-1};

// Main entry point of the program
int main(int argc, char* argv[])
{
  if (argc != 4)
  {
    std::cerr << "Usage: " << argv[0] << " <port> <speed_milliseconds> <file_path>" << std::endl;
    return 1;
  }

  MessageHandler message_handler;
  ConnectionManager peer{message_handler};

  std::thread peerEventThread {&ConnectionManager::runEventLoop, &peer};
  peer.initListeners(std::stoi(argv[1]));
  peer.startListening();

  socket_descriptor.store(peer.connect("localhost", std::stoi(argv[1])));

  if (socket_descriptor < 0)
  {
    std::cout << "Error connecting to the server." << std::endl;
    return 1;
  }

  std::cin.get();

  std::thread sendThread {[&peer, argv](){
    std::fstream file{argv[3], std::ios::in};
    if (!(file.is_open()))
    {
      std::cerr << "Error opening the file." << std::endl;
      return;
    }
    char ch;
    while (file.get(ch) && is_running.load())
    {
      int send_status;
      send_status = peer.send(
        socket_descriptor,
        MessageType::MESSAGE,
        std::string(1, ch).c_str(),
        1
      );
      if (send_status < 0)
        std::cout << "Error sending the message to server." << std::endl;
      std::this_thread::sleep_for(std::chrono::milliseconds(std::stoi(argv[2])));
    }
  }};

  std::cin.get();

  is_running.store(false);

  peer.disconnect(socket_descriptor);
  peer.stopListening();

  peer.stopEventLoop();

  sendThread.join();
  peerEventThread.join();

  return 0;
}
