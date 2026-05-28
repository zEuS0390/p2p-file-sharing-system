#include <iostream>
#include <memory>

#include "core/network/ClientMessageHandler.hpp"
#include "core/types/MessageHeaders.hpp"

void ClientMessageHandler::dispatchMessage(
 std::shared_ptr<Connection> connection,
 MessageHeader& message_header,
 const char* data
)
{
  switch (message_header.type)
  {
    case MessageType::MESSAGE:
    {
      std::cout << data;
      std::cout.flush();
      break;
    }
    default:
      break;
  }
}

