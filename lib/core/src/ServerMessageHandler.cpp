#include <sys/socket.h>
#include <cstring>
#include <fstream>
#include <string>
#include <iostream>
#include <ios>

#include "core/network/ServerMessageHandler.hpp"
#include "core/types/MessageHeaders.hpp"
#include "core/types/FileErrorCode.hpp"
#include "core/types/MessageType.hpp"
#include "core/types/Connection.hpp"
#include "core/types/OutgoingFileTransfer.hpp"

void ServerMessageHandler::dispatchMessage(
 Connection& connection,
 const char* data
)
{
  switch (connection.current_header.type)
  {
    case MessageType::MESSAGE:
    {
      queueMessage(
        connection,
        MessageType::MESSAGE,
        data,
        connection.current_header.payload_size
      );
      break;
    }
    case MessageType::FILE_INFO_REQUEST:
    {
      FileRequestHeader file_request_header {};
      std::memcpy(&file_request_header, data, sizeof(FileRequestHeader));
      const char* file_name {data + sizeof(file_request_header)};
      std::string file_name_str {file_name, file_request_header.filename_size};
      std::ifstream input_file_stream {file_name_str, std::ios::binary | std::ios::ate};

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
      std::memcpy(payload.data() + sizeof(FileInfoHeader), file_name_str.c_str(), file_request_header.filename_size);

      queueMessage(
        connection,
        MessageType::FILE_INFO,
        payload.data(),
        payload.size()
      );

      break;
    }
    case MessageType::FILE_DOWNLOAD_REQUEST:
    {
      FileRequestHeader file_request_header {};
      std::memcpy(&file_request_header, data, sizeof(FileRequestHeader));
      const char* file_name {data + sizeof(FileRequestHeader)};
      std::string file_name_str {file_name, file_request_header.filename_size};

      // Verify the file 
      std::ifstream input_file_stream {file_name_str, std::ios::binary | std::ios::ate};
      if (!input_file_stream.is_open())
      {
        std::string error_message {std::strerror(errno)};
        FileErrorHeader file_error_header {};
        file_error_header.error_code = FileErrorCode::FILE_GENERIC_ERROR;
        file_error_header.message_size = error_message.size();
        std::vector<char> payload;
        payload.reserve(error_message.size() + sizeof(FileErrorHeader));
        payload.resize(error_message.size() + sizeof(FileErrorHeader));
        std::memcpy(payload.data(), &file_error_header, sizeof(FileErrorHeader));
        std::memcpy(payload.data() + sizeof(FileErrorHeader), error_message.data(), error_message.size());
        queueMessage(connection, MessageType::FILE_ERROR, payload.data(), payload.size());
        input_file_stream.close();
        break;
      }

      // Get the file size
      std::streamsize file_size {input_file_stream.tellg()};

      // Clear error / EOF flags and move pointer back to start
      input_file_stream.clear();
      input_file_stream.seekg(0, std::ios::beg);

      const std::uint64_t transfer_id {++connection.transfer_id};
      OutgoingFileTransfer outgoing_file;
      outgoing_file.file = std::move(input_file_stream);
      outgoing_file.file_size = file_size;
      outgoing_file.filename = file_name_str;
      connection.outgoing_files.emplace(transfer_id, std::move(outgoing_file));

      FileInfoHeader file_info_header;
      file_info_header.transfer_id = transfer_id;
      file_info_header.filename_size = file_request_header.filename_size;
      file_info_header.file_size = file_size;

      std::vector<char> payload;
      payload.resize(sizeof(FileInfoHeader) + file_name_str.size());
      std::memcpy(payload.data(), &file_info_header, sizeof(FileInfoHeader));
      std::memcpy(payload.data() + sizeof(FileInfoHeader), file_name_str.data(), file_name_str.size());

      queueMessage(
        connection,
        MessageType::FILE_INFO,
        payload.data(),
        payload.size()
      );

      break;
    }
    case MessageType::FILE_ACK:
    {
      FileAckHeader file_ack_header {};
      std::memcpy(&file_ack_header, data, sizeof(FileAckHeader));

      auto it {connection.outgoing_files.find(file_ack_header.transfer_id)};

      if (it != connection.outgoing_files.end())
      {
        OutgoingFileTransfer& outgoing_file = it->second;

        constexpr std::size_t CHUNK_SIZE {4096};
        std::vector<char> file_chunk (CHUNK_SIZE);
        outgoing_file.file.read(file_chunk.data(), file_chunk.size());

        std::streamsize bytes_read = outgoing_file.file.gcount();

        if (bytes_read > 0)
        {
          FileChunkHeader file_chunk_header {};
          file_chunk_header.transfer_id = file_ack_header.transfer_id;
          file_chunk_header.chunk_size = bytes_read;
          std::vector<char> payload;
          payload.resize(sizeof(FileChunkHeader) + bytes_read);
          std::memcpy(payload.data(), &file_chunk_header, sizeof(FileChunkHeader));
          std::memcpy(payload.data()+sizeof(FileChunkHeader), file_chunk.data(), file_chunk.size());
          queueMessage(
            connection,
            MessageType::FILE_CHUNK,
            payload.data(),
            payload.size()
          );
        }
      }
      break;
    }
    default:
      break;
  }
}

void ServerMessageHandler::queueMessage(
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

