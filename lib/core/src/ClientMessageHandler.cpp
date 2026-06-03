#include <iostream>
#include <cstring>
#include <memory>
#include <sys/poll.h>

#include "core/network/ClientMessageHandler.hpp"
#include "core/types/MessageHeaders.hpp"
#include "core/types/MessageType.hpp"
#include "core/types/Connection.hpp"

void ClientMessageHandler::dispatchMessage(
 std::shared_ptr<Connection> connection,
 std::shared_ptr<pollfd> connection_pollfd,
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
    case MessageType::FILE_INFO:
    {
      FileInfoHeader file_info_header;

      std::memcpy(&file_info_header, data, sizeof(FileInfoHeader));

      const char* file_name {data + sizeof(FileInfoHeader)};

      std::string file_name_str {
        file_name,
        file_info_header.filename_size
      };
      std::cout << "filename: " << file_name_str << std::endl;
      std::cout << "filename size: " << file_info_header.filename_size << std::endl;
      std::cout << "file_size: " << file_info_header.file_size << std::endl;
    }
    default:
      break;
  }
}

void ClientMessageHandler::queueMessage(
  std::shared_ptr<Connection> connection,
  std::shared_ptr<pollfd> connection_pollfd,
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

  connection_pollfd->events |= POLLOUT;
}
