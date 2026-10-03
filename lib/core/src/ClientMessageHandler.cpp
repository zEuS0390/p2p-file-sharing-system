#include <filesystem>
#include <fstream>
#include <iomanip>
#include <ios>
#include <iostream>
#include <cstring>
#include <string>

#include "core/network/ClientMessageHandler.hpp"
#include "core/types/IncomingFileTransfer.hpp"
#include "core/types/MessageHeaders.hpp"
#include "core/types/MessageType.hpp"
#include "core/types/Connection.hpp"

void ClientMessageHandler::dispatchMessage(
 Connection& connection,
 const char* data
)
{
  switch (connection.current_header.type)
  {
    case MessageType::MESSAGE:
    {
      std::string data_str {data, connection.current_header.payload_size};
      std::cout << data_str;
      std::cout.flush();
      break;
    }
    case MessageType::FILE_INFO:
    {
      FileInfoHeader file_info_header {};
      std::memcpy(&file_info_header, data, sizeof(FileInfoHeader));
      const char* file_name {data + sizeof(FileInfoHeader)};
      std::string file_name_str {file_name, file_info_header.filename_size};

      std::cout << "Filename: " << file_name_str << std::endl;
      std::cout << "Filename Byte Size: " << file_info_header.filename_size << std::endl;
      std::cout << "File Byte Size: " << file_info_header.file_size << std::endl;
      std::cout << "File Transfer ID: " << file_info_header.transfer_id << std::endl;

      std::filesystem::path file_path {file_name_str};
      std::string output_file_name_str {file_path.filename().string()};
      IncomingFileTransfer incominng_file_transfer;
      incominng_file_transfer.file.open(output_file_name_str, std::ios::binary | std::ios::trunc);
      incominng_file_transfer.filename = output_file_name_str;
      incominng_file_transfer.file_size = file_info_header.file_size;
      incominng_file_transfer.bytes_received = 0;
      connection.incoming_files.emplace(
        file_info_header.transfer_id,
        std::move(incominng_file_transfer)
      );

      FileAckHeader file_ack_header {};
      file_ack_header.transfer_id = file_info_header.transfer_id;

      std::vector<char> payload;
      payload.resize(sizeof(FileAckHeader));
      std::memcpy(payload.data(), &file_ack_header, sizeof(FileAckHeader));

      queueMessage(
        connection,
        MessageType::FILE_ACK,
        payload.data(),
        payload.size()
      );

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

        // NOTE: This is just for displaying one progress for the incoming file transfer.
        std::cout << "\r\033[2K"
                  << incoming_file.bytes_received
                  << "/"
                  << incoming_file.file_size
                  << " - "
                  << ((static_cast<double>(incoming_file.bytes_received)/static_cast<double>(incoming_file.file_size))*100)
                  << "%"
                  << std::fixed
                  << std::setprecision(2)
                  << std::flush;

        if (incoming_file.bytes_received == incoming_file.file_size)
        {
          std::cout << "\nFile download complete.\n" << std::flush;
          incoming_file.file.close();
          connection.incoming_files.erase(it);
          FileEndHeader file_end_header {};
          file_end_header.transfer_id = file_chunk_header.transfer_id;
          std::vector<char> payload;
          payload.resize(sizeof(FileEndHeader));
          std::memcpy(payload.data(), &file_end_header, sizeof(FileEndHeader));
          queueMessage(
            connection,
            MessageType::FILE_END,
            payload.data(),
            payload.size()
          );
          break;
        }

        FileAckHeader file_ack_header {};
        file_ack_header.transfer_id = file_chunk_header.transfer_id;
        std::vector<char> payload;
        payload.resize(sizeof(FileAckHeader));
        std::memcpy(payload.data(), &file_ack_header, sizeof(FileAckHeader));
        queueMessage(
          connection,
          MessageType::FILE_ACK,
          payload.data(),
          payload.size()
        );
      }
      break;
    }
    case MessageType::FILE_ERROR:
    {
      FileErrorHeader file_error_header;
      std::memcpy(&file_error_header, data, sizeof(FileErrorHeader));
      const char* message {data + sizeof(FileErrorHeader)};
      std::cout << message << "\n" << std::flush;
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

