#include <sys/socket.h>
#include <cstring>
#include <fstream>
#include <string>
#include <iostream>
#include <memory>
#include <ios>

#include "core/network/ServerMessageHandler.hpp"
#include "core/types/MessageHeaders.hpp"
#include "core/types/FileErrorCode.hpp"
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
    case MessageType::FILE_REQUEST:
    {
      FileRequestHeader file_request_header {};
      std::memcpy(&file_request_header, data, message_header.payload_size);
      const char* file_name {data + sizeof(file_request_header)};
      std::ifstream input_file_stream {file_name, std::ios::binary | std::ios::ate};

      // Check if oopening the file was not successful
      if (!input_file_stream.is_open())
      {
        std::string buffer {std::strerror(errno)};
        FileErrorHeader file_error_header;
        file_error_header.error_code = FileErrorCode::FILE_GENERIC_ERROR;
        file_error_header.message_size = buffer.size();
        std::vector<char> payload;
        payload.resize(sizeof(FileErrorHeader) + buffer.size());
        std::memcpy(payload.data(), &file_error_header, sizeof(FileErrorHeader));
        std::memcpy(payload.data() + sizeof(FileErrorHeader), buffer.c_str(), buffer.size());
        queueMessage(connection, MessageType::FILE_ERROR, payload.data(), payload.size());
        input_file_stream.close();
        break;
      }

      std::streamsize size {input_file_stream.tellg()};
      input_file_stream.close();

      FileInfoHeader file_information_header;
      file_information_header.filename_size = file_request_header.filename_size;
      file_information_header.file_size = size;

      std::vector<char> payload;

      payload.resize(sizeof(file_information_header) + file_request_header.filename_size);

      std::memcpy(payload.data(), &file_information_header, sizeof(FileInfoHeader));
      std::memcpy(payload.data() + sizeof(FileInfoHeader), file_name, file_request_header.filename_size);

      // Send file information
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
