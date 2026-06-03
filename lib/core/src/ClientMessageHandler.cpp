#include <iostream>
#include <cstring>
#include <memory>

#include "core/network/ClientMessageHandler.hpp"
#include "core/types/MessageHeaders.hpp"
#include "core/types/MessageType.hpp"
#include "core/types/Connection.hpp"

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

void ClientMessageHandler::queueMessage(
  std::shared_ptr<Connection> connection,
  const MessageType& message_type,
  const char* data,
  size_t length
)
{
  MessageHeader message_header;
  message_header.type = message_type;
  message_header.payload_size = length;

  size_t old_size {connection->send_buffer.size()};

  connection->send_buffer.resize(old_size + sizeof(message_header) + length);

  memcpy(
    connection->send_buffer.data() + old_size,
    &message_header,
    sizeof(message_header)
  );

  memcpy(
    connection->send_buffer.data() + old_size + sizeof(message_header),
    data,
    length
  );
}
