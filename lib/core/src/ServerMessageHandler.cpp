#include <iostream>
#include <memory>
#include <sys/socket.h>

#include "core/network/ServerMessageHandler.hpp"
#include "core/types/MessageHeaders.hpp"
#include "core/types/MessageType.hpp"
#include "core/types/Connection.hpp"

void ServerMessageHandler::dispatchMessage(
 std::shared_ptr<Connection> connection,
 MessageHeader& message_header,
 const char* data
)
{
  switch (message_header.type)
  {
    case MessageType::MESSAGE:
    {
      MessageHeader send_message_header;
      send_message_header.type = MessageType::MESSAGE;
      send_message_header.payload_size = message_header.payload_size;

      ssize_t send_status {0};

      send_status = send(
        connection->endpoint.socket_descriptor,
        reinterpret_cast<char*>(&send_message_header),
        sizeof(send_message_header),
        MSG_NOSIGNAL
      );

      if (send_status <= 0)
      {
        std::cout << "error sending the message header." << std::endl;
        std::cout.flush();
        break;
      }

      send_status = send(
        connection->endpoint.socket_descriptor,
        data,
        message_header.payload_size,
        MSG_NOSIGNAL
      );

      if (send_status <= 0)
      {
        std::cout << "error sending the message payload." << std::endl;
        std::cout.flush();
        break;
      }

      // std::cout << data;
      // std::cout.flush();

      break;
    }
    default:
      break;
  }
}
