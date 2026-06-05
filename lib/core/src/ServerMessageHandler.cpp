#include <sys/socket.h>
#include <cstring>
#include <string>
#include <memory>

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
      queueMessage(
        connection,
        MessageType::MESSAGE,
        data,
        message_header.payload_size
      );
      break;
    }
    case MessageType::FILE_INFO:
    {
      std::string file_name {"sample.mp4"};
      uint64_t file_size {4096};

      FileInfoHeader file_information_header;
      file_information_header.filename_size = file_name.size();
      file_information_header.file_size = file_size;

      std::vector<char> payload;

      payload.resize(sizeof(file_information_header) + file_name.size() + 1);

      std::memcpy(payload.data(), &file_information_header, sizeof(file_information_header));
      std::memcpy(payload.data() + sizeof(file_information_header), file_name.data(), file_name.size() + 1);

      queueMessage(
        connection,
        MessageType::FILE_INFO,
        payload.data(),
        payload.size()
      );

      break;
    }
    default:
      break;
  }
}

void ServerMessageHandler::queueMessage(
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

  std::memcpy(
    connection->send_buffer.data() + old_size,
    &message_header,
    sizeof(message_header)
  );

  std::memcpy(
    connection->send_buffer.data() + old_size + sizeof(message_header),
    data,
    length
  );

  connection->pollfd.events |= POLLOUT;
}
