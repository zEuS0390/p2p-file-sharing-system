#include <iostream>
#include <fstream>
#include <string>

#include "core/network/ClientMessageHandler.hpp"
#include "core/network/ServerMessageHandler.hpp"
#include "core/types/MessageType.hpp"
#include "core/network/Peer.hpp"

// Main entry point of the program
int main(int argc, char* argv[])
{
  if (argc != 4)
  {
    std::cerr << "Usage: " << argv[0] << " <port> <speed_milliseconds> <file_path>" << std::endl;
    return 1;
  }

  ServerMessageHandler server_message_handler;
  ClientMessageHandler client_message_handler;

  Peer peer{server_message_handler, client_message_handler};

  peer.start(std::stoi(argv[1]));

  int socket_descriptor {peer.connect("localhost", std::stoi(argv[1]))};

  std::cin.get();

  std::fstream file{argv[3], std::ios::in};

  if (!(file.is_open()))
  {
    std::cerr << "Error opening the file." << std::endl;
    return 1;
  }

  char ch;
  while (file.get(ch))
  {
    MessageHeader message_header;
    message_header.type = MessageType::MESSAGE;
    message_header.payload_size = 1;

    ssize_t send_status;

    send_status = peer.send(
      socket_descriptor,
      MessageType::MESSAGE,
      reinterpret_cast<char*>(&message_header),
      sizeof(message_header)
    );

    if (send_status < 0)
    {
      std::cout << "Error sending the message to server." << std::endl;
      break;
    }

    send_status = peer.send(
        socket_descriptor,
        MessageType::MESSAGE,
        std::string(1, ch).c_str(),
        message_header.payload_size
    );

    if (send_status < 0)
    {
      std::cout << "Error sending the message to server." << std::endl;
      break;
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(std::stoi(argv[2])));
  }

  std::cout << "Press enter to continue..." << std::endl;
  std::cin.get();

  peer.disconnect(socket_descriptor);

  peer.stop();

  return 0;
}
