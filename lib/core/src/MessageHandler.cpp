#include <sys/socket.h>
#include <string_view>
#include <filesystem>
#include <iostream>
#include <cstring>
#include <fstream>
#include <string>
#include <ios>

#include "core/types/OutgoingFileTransfer.hpp"
#include "core/types/IncomingFileTransfer.hpp"
#include "core/network/MessageHandler.hpp"
#include "core/types/MessageHeaders.hpp"
#include "core/types/FileErrorCode.hpp"
#include "core/types/MessageType.hpp"
#include "core/types/Connection.hpp"

void MessageHandler::appendBytes(std::vector<char>& buffer, const void* data, std::size_t size_t)
{
  const std::size_t offset {buffer.size()};
  buffer.resize(offset + size_t);
  std::memcpy(buffer.data() + offset, data, size_t);
}

void MessageHandler::dispatchMessage(Connection& connection, const char* data)
{
  switch (connection.current_header.type)
  {
    case MessageType::MESSAGE_REQUEST:
    {
      MessageRequestHeader message_request_header {};
      std::memcpy(&message_request_header, data, sizeof(MessageRequestHeader));

      MessageResponseHeader message_reponse_header {
        .request_id = message_request_header.request_id,
        .message_size = message_request_header.message_size
      };

      std::vector<char> payload;
      appendBytes(payload, &message_reponse_header, sizeof(message_reponse_header));
      appendBytes(payload, data+sizeof(MessageRequestHeader), message_request_header.message_size);
      queueMessage(connection, MessageType::MESSAGE_RESPONSE, payload.data(), payload.size());

      break;
    }
    case MessageType::MESSAGE_RESPONSE:
    {
      MessageResponseHeader message_reponse_header {};
      std::memcpy(&message_reponse_header, data, connection.current_header.payload_size);
      std::string_view message {data+sizeof(MessageResponseHeader), message_reponse_header.message_size};
      auto& handler = std::get<message_handler_detail::HandlerSlot<MessageType::MESSAGE_RESPONSE> >(handlers).function;
      if (handler)
        handler(message_reponse_header.request_id, message);
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
        FileErrorHeader file_error_header {
            .error_code = FileErrorCode::FILE_GENERIC_ERROR,
            .message_size = buffer.size()
        };
        std::vector<char> payload;
        appendBytes(payload, &file_error_header, sizeof(FileErrorHeader));
        appendBytes(payload, buffer.data(), buffer.size());
        queueMessage(connection, MessageType::FILE_ERROR, payload.data(), payload.size());
        input_file_stream.close();
        break;
      }

      std::streamsize size {input_file_stream.tellg()};
      input_file_stream.close();

      std::string message;
      message += "Filename: " + file_name_str + "\n";
      message += "Filename Byte Size: " + std::to_string(file_name_str.size()) + "\n";
      message += "File Byte Size: " + std::to_string(size) + "\n";

      queueMessage(connection, MessageType::MESSAGE_RESPONSE, message.c_str(), message.size());

      break;
    }
    case MessageType::FILE_ERROR:
    {
      FileErrorHeader file_error_header;
      std::memcpy(&file_error_header, data, sizeof(FileErrorHeader));
      std::string_view error_message {data+sizeof(FileErrorHeader), file_error_header.message_size};
      auto& handler = std::get<message_handler_detail::HandlerSlot<MessageType::FILE_ERROR> >(handlers).function;
      if (handler)
        handler(file_error_header.transfer_id, error_message);
      break;
    }
    case MessageType::FILE_DOWNLOAD_REQUEST:
    {
      FileDownloadRequestHeader file_download_request_header {};
      std::memcpy(&file_download_request_header, data, sizeof(FileDownloadRequestHeader));
      const char* file_name {data + sizeof(FileDownloadRequestHeader)};
      std::string file_name_str {file_name, file_download_request_header.filename_size};

      // Verify the file 
      std::ifstream input_file_stream {file_name_str, std::ios::binary | std::ios::ate};
      if (!input_file_stream.is_open())
      {
        std::string_view error_message {std::strerror(errno)};
        FileErrorHeader file_error_header {
          .transfer_id = 0,
          .error_code = FileErrorCode::FILE_GENERIC_ERROR,
          .message_size = error_message.size()
        };
        std::vector<char> payload;
        appendBytes(payload, &file_error_header, sizeof(FileErrorHeader));
        appendBytes(payload, error_message.data(), error_message.size());
        queueMessage(connection, MessageType::FILE_ERROR, payload.data(), payload.size());
        input_file_stream.close();
        break;
      }

      // Get the file size. Implicitply convert from std::streamsize to std::uint64_t
      const std::uint64_t file_size = input_file_stream.tellg();

      // Clear error / EOF flags and move pointer back to start
      input_file_stream.clear();
      input_file_stream.seekg(0, std::ios::beg);

      const std::uint64_t transfer_id {++connection.transfer_id};
      OutgoingFileTransfer outgoing_file {
        .file = std::move(input_file_stream),
        .filename = file_name_str,
        .file_size = file_size
      };
      connection.outgoing_files.emplace(transfer_id, std::move(outgoing_file));

      FileDownloadResponseHeader file_download_response_header {
        .request_id = file_download_request_header.request_id,
        .transfer_id = transfer_id,
        .filename_size = file_download_request_header.filename_size,
        .file_size = file_size
      };

      std::vector<char> payload;
      appendBytes(payload, &file_download_response_header, sizeof(FileDownloadResponseHeader));
      appendBytes(payload, file_name_str.c_str(), file_name_str.size());
      queueMessage(connection, MessageType::FILE_DOWNLOAD_RESPONSE, payload.data(), payload.size());

      break;
    }
    case MessageType::FILE_DOWNLOAD_RESPONSE:
    {
      FileDownloadResponseHeader file_download_response_header {};
      std::memcpy(&file_download_response_header, data, sizeof(FileDownloadResponseHeader));
      const char* file_name {data + sizeof(FileDownloadResponseHeader)};
      std::string_view file_name_str {file_name, file_download_response_header.filename_size};

      std::filesystem::path file_path {file_name_str};
      std::string output_file_name_str {file_path.filename().string()};

      IncomingFileTransfer incominng_file_transfer;
      incominng_file_transfer.file.open(output_file_name_str, std::ios::binary | std::ios::trunc);
      incominng_file_transfer.filename = output_file_name_str;
      incominng_file_transfer.file_size = file_download_response_header.file_size;
      incominng_file_transfer.bytes_received = 0;

      connection.incoming_files.emplace(file_download_response_header.transfer_id, std::move(incominng_file_transfer));

      auto& handler = std::get<message_handler_detail::HandlerSlot<MessageType::FILE_DOWNLOAD_RESPONSE> >(handlers).function;
      if (handler)
        handler(file_download_response_header.transfer_id, file_download_response_header.file_size);

      break;
    }
    case MessageType::FILE_UPLOAD_REQUEST:
    {
      FileUploadRequestHeader file_upload_request_header {};
      std::memcpy(&file_upload_request_header, data, sizeof(FileUploadRequestHeader));
      const char* file_name {data + sizeof(FileUploadRequestHeader)};
      std::string_view file_name_str {file_name, file_upload_request_header.filename_size};

      const std::uint64_t transfer_id {++connection.transfer_id};

      std::filesystem::path file_path {file_name_str};
      std::string output_file_name_str {file_path.filename().string()};
      std::ofstream output_file_stream {output_file_name_str, std::ios::binary | std::ios::trunc};

      if (!output_file_stream.is_open())
      {
        std::string_view error_message {std::strerror(errno)};
        FileErrorHeader file_error_header {};
        file_error_header.error_code = FileErrorCode::FILE_GENERIC_ERROR;
        file_error_header.message_size = error_message.size();
        std::vector<char> payload;
        appendBytes(payload, &file_error_header, sizeof(FileErrorHeader));
        appendBytes(payload, error_message.data(), error_message.size());
        queueMessage(connection, MessageType::FILE_ERROR, payload.data(), payload.size());
        output_file_stream.close();
        break;
      }

      IncomingFileTransfer incoming_file;
      incoming_file.file = std::move(output_file_stream);
      incoming_file.filename = file_name_str;
      incoming_file.file_size = file_upload_request_header.file_size;
      incoming_file.bytes_received = 0;

      connection.incoming_files.emplace(transfer_id, std::move(incoming_file));

      FileUploadResponseHeader file_upload_info_header;
      file_upload_info_header.transfer_id = transfer_id;
      file_upload_info_header.filename_size = file_name_str.size();

      std::vector<char> payload;
      appendBytes(payload, &file_upload_info_header, sizeof(FileUploadResponseHeader));
      appendBytes(payload, file_name_str.data(), file_name_str.size());

      queueMessage(connection, MessageType::FILE_UPLOAD_RESPONSE, payload.data(), payload.size());

      break;
    }
    case MessageType::FILE_UPLOAD_RESPONSE:
    {
      FileUploadResponseHeader file_info_header {};
      std::memcpy(&file_info_header, data, sizeof(FileUploadResponseHeader));
      const char* file_name {data + sizeof(FileUploadResponseHeader)};
      std::string file_name_str {file_name, file_info_header.filename_size};
      std::ifstream input_file_stream {file_name_str, std::ios::binary | std::ios::ate};

      if (!input_file_stream.is_open())
      {
        std::string_view error_message {std::strerror(errno)};
        FileErrorHeader file_error_header {
          .error_code = FileErrorCode::FILE_GENERIC_ERROR,
          .message_size = error_message.size()
        };
        std::vector<char> payload;
        appendBytes(payload, &file_error_header, sizeof(FileErrorHeader));
        appendBytes(payload, error_message.data(), error_message.size());
        queueMessage(connection, MessageType::FILE_ERROR, payload.data(), payload.size());
        input_file_stream.close();
        break;
      }

      const std::uint64_t file_size = input_file_stream.tellg();

      // Clear error / EOF flags and move pointer back to start
      input_file_stream.clear();
      input_file_stream.seekg(0, std::ios::beg);

      OutgoingFileTransfer outgoing_file {std::move(input_file_stream), file_name_str, file_size, 0};

      connection.outgoing_files.emplace(
        file_info_header.transfer_id,
        std::move(outgoing_file)
      );

      auto& handler = std::get<message_handler_detail::HandlerSlot<MessageType::FILE_UPLOAD_RESPONSE> >(handlers).function;
      if (handler)
        handler(file_info_header.transfer_id);

      break;
    }
    case MessageType::FILE_CHUNK:
    {
      FileChunkHeader file_chunk_header {};
      std::memcpy(&file_chunk_header, data, sizeof(FileChunkHeader));
      const char* file_chunk = data + sizeof(FileChunkHeader);
      auto it {connection.incoming_files.find(file_chunk_header.transfer_id)};
      if (it != connection.incoming_files.end())
      {
        IncomingFileTransfer& incoming_file {it->second};
        incoming_file.file.write(file_chunk, file_chunk_header.chunk_size);
        incoming_file.bytes_received += file_chunk_header.chunk_size;
        if (incoming_file.bytes_received == incoming_file.file_size)
        {
          incoming_file.file.close();
          connection.incoming_files.erase(it);
          FileEndHeader file_end_header {file_chunk_header.transfer_id};
          std::vector<char> payload;
          appendBytes(payload, &file_end_header, sizeof(FileEndHeader));
          queueMessage(connection, MessageType::FILE_END, payload.data(), payload.size());
        }
      }
      auto& handler = std::get<message_handler_detail::HandlerSlot<MessageType::FILE_CHUNK> >(handlers).function;
      if (handler)
        handler(file_chunk_header.transfer_id, file_chunk_header.chunk_size);
      break;
    }
    case MessageType::FILE_END:
    {
      FileEndHeader file_end_header {};
      std::memcpy(&file_end_header, data, sizeof(FileEndHeader));
      auto it {connection.outgoing_files.find(file_end_header.transfer_id)};
      if (it != connection.outgoing_files.end())
      {
        OutgoingFileTransfer& outgoing_file {it->second};
        outgoing_file.file.close();
        connection.outgoing_files.erase(it);
      }
      break;
    }
    default:
      break;
  }
}

void MessageHandler::queueMessage(
  Connection& connection,
  const MessageType& message_type,
  const char* data,
  std::size_t length
)
{
  MessageHeader message_header {};
  message_header.payload_size = length;
  message_header.type = message_type,
  appendBytes(connection.send_buffer, &message_header, sizeof(MessageHeader));
  appendBytes(connection.send_buffer, data, length);
}

bool MessageHandler::queueNextFileChunk(Connection &connection, std::uint64_t transfer_id)
{
  auto it {connection.outgoing_files.find(transfer_id)};
  if (it == connection.outgoing_files.end())
    return false;
  constexpr std::uint64_t MAX_CHUNK_SIZE {1024 * 64};
  OutgoingFileTransfer& outgoing_file {it->second};
  if (outgoing_file.bytes_sent >= outgoing_file.file_size)
  {
    FileEndHeader file_end_header {};
    file_end_header.transfer_id = transfer_id;
    std::vector<char> payload (sizeof(FileEndHeader));
    std::memcpy(payload.data(), &file_end_header, sizeof(FileEndHeader));
    queueMessage(connection, MessageType::FILE_END, payload.data(), payload.size());
    return true;
  }
  const std::uint64_t remaining {outgoing_file.file_size - outgoing_file.bytes_sent};
  const std::uint64_t chunk_size {std::min(remaining, MAX_CHUNK_SIZE)};
  std::vector<char> payload (sizeof(FileChunkHeader) + chunk_size);
  FileChunkHeader file_chunk_header {
    .transfer_id = transfer_id,
    .chunk_size = chunk_size
  };
  std::memcpy(payload.data(), &file_chunk_header, sizeof(FileChunkHeader));
  outgoing_file.file.read(payload.data() + sizeof(FileChunkHeader), chunk_size);
  std::streamsize bytes_read {outgoing_file.file.gcount()};
  if (bytes_read <= 0)
    return false;
  payload.resize(sizeof(FileChunkHeader) + bytes_read);
  queueMessage(connection, MessageType::FILE_CHUNK, payload.data(), payload.size());
  outgoing_file.bytes_sent += bytes_read;
  auto& handler {std::get<message_handler_detail::HandlerSlot<MessageType::FILE_UPLOAD_PROGRESS> >(handlers).function};
  if (handler)
    handler(transfer_id, outgoing_file.bytes_sent, outgoing_file.file_size);
  return true;
}
