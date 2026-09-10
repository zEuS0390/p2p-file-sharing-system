#include <iostream>
#include <cstring>
#include <string>

#include "core/network/ClientMessageHandler.hpp"
#include "core/types/MessageHeaders.hpp"
#include "core/types/MessageType.hpp"
#include "core/types/Connection.hpp"

void ClientMessageHandler::dispatchMessage(
 Connection& connection,
 MessageHeader& message_header,
 const char* data
)
{
  switch (message_header.type)
  {
    case MessageType::MESSAGE:
    {
      std::string data_str {data, message_header.payload_size};
      std::cout << data_str;
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
      std::cout << "Filename: " << file_name_str << std::endl;
      std::cout << "Filename Byte Size: " << file_info_header.filename_size << std::endl;
      std::cout << "File Byte Size: " << file_info_header.file_size << std::endl;
      break;
    }
    case MessageType::FILE_ERROR:
    {
      FileErrorHeader file_error_header;
      std::memcpy(&file_error_header, data, sizeof(FileErrorHeader));
      const char* message {data + sizeof(FileErrorHeader)};
      std::cout << message << std::endl;
      break;
    }
    default:
      break;
  }
}

void ClientMessageHandler::queueMessage(
  Connection& connection,
  const MessageType& message_type,
  const char* data,
  size_t length
)
{
  MessageHeader message_header;
  message_header.type = message_type;
  message_header.payload_size = length;

  size_t old_size {connection.send_buffer.size()};

  connection.send_buffer.resize(old_size + sizeof(message_header) + length);

  std::memcpy(
    connection.send_buffer.data() + old_size,
    &message_header,
    sizeof(message_header)
  );

  std::memcpy(
    connection.send_buffer.data() + old_size + sizeof(message_header),
    data,
    length
  );
}

