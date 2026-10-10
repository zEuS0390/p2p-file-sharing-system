#include <cstdint>
#include <iostream>
#include <cstdlib>
#include <string>
#include <cstring>
#include <fstream>
#include <string_view>
#include <thread>
#include <chrono>
#include <atomic>
#include <ios>

#include "core/network/MessageHandler.hpp"
#include "core/network/ConnectionManager.hpp"
#include "core/types/MessageHeaders.hpp"
#include "core/types/MessageType.hpp"

std::atomic<bool> is_running {true};
std::atomic<int> socket_descriptor {-1};

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

  MessageHandler message_handler;

  message_handler.on<MessageType::MESSAGE_RESPONSE>(
    [](std::uint64_t request_id, std::string_view str_view)
    {
      std::cout << str_view << std::flush;
    }
  );

  ConnectionManager client {message_handler};

  socket_descriptor.store(client.connect(argv[1], atoi(argv[2])));

  if (socket_descriptor < 0)
  {
    std::cout << "Error connecting to the server." << std::endl;
    return 1;
  }

  std::cout << "Press enter to continue..." << std::endl;
  std::cin.get();

  std::thread evenThread {&ConnectionManager::runEventLoop, &client};

  std::thread sendThread {[&client, argv](){
    std::fstream file{argv[4], std::ios::in};
    if (!(file.is_open()))
    {
      std::cerr << "Error opening the file." << std::endl;
      return;
    }
    char ch;
    while (file.get(ch) && is_running.load())
    {
      MessageRequestHeader message_request_header {};
      message_request_header.request_id = 1;
      message_request_header.message_size = 1;

      std::vector<char> payload;
      payload.resize(sizeof(MessageRequestHeader) + 1);
      std::memcpy(payload.data(), &message_request_header, sizeof(MessageRequestHeader));
      std::memcpy(payload.data()+sizeof(MessageRequestHeader), std::string(1, ch).c_str(), 1);

      int send_status;
      send_status = client.send(
        socket_descriptor,
        MessageType::MESSAGE_REQUEST,
        payload.data(),
        payload.size()
      );

      if (send_status < 0)
      {
        std::cout << "Error sending the message to server." << std::endl;
        break;
      }

      std::this_thread::sleep_for(std::chrono::milliseconds(std::stoi(argv[3])));
    }
  }};

  std::cin.get();

  is_running.store(false);

  client.disconnect(socket_descriptor);

  client.stopEventLoop();

  sendThread.join();
  evenThread.join();

  return 0;
}

