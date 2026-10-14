#include <unordered_map>
#include <iostream>
#include <csignal>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <thread>

#include "core/network/ConnectionManager.hpp"
#include "core/network/MessageHandler.hpp"
#include "core/types/MessageHeaders.hpp"
#include "core/types/MessageType.hpp"

volatile sig_atomic_t shutdown_requested = 0;

void handle_signal(int)
{
  shutdown_requested = 1;
}

struct FileUploadProgress
{
  std::uint64_t file_size {0};
  std::uint64_t bytes_sent {0};
};

// Main Entry Point of the Program
int main(int argc, const char** argv)
{
  if (argc < 4)
  {
    std::cerr << "Usage: "
              << argv[0]
              << " <hostname> <port> <filename1> <filename2> ..."
              << std::endl;
    return 1;
  }

  std::signal(SIGTERM, handle_signal);
  std::signal(SIGINT, handle_signal);

  MessageHandler message_handler;

  ConnectionManager client {message_handler};

  std::unordered_map<std::uint64_t, FileUploadProgress> file_uploads;

  message_handler.on<MessageType::FILE_UPLOAD_RESPONSE>(
    [&file_uploads](std::uint64_t transfer_id)
    {
      std::cout << "File upload requested." << std::endl;
      file_uploads.emplace(transfer_id, FileUploadProgress{0, 0});
    }
  );

  message_handler.on<MessageType::FILE_UPLOAD_PROGRESS>(
    [&file_uploads](std::uint64_t transfer_id, std::uint64_t bytes_sent, std::uint64_t file_size)
    {
      auto it {file_uploads.find(transfer_id)};
      if (it == file_uploads.end())
        return;
      file_uploads[transfer_id].bytes_sent = bytes_sent;
      file_uploads[transfer_id].file_size = file_size;
      if (file_uploads[transfer_id].bytes_sent == file_uploads[transfer_id].file_size)
        file_uploads.erase(transfer_id);
    }
  );

  std::unordered_map<std::uint64_t, FileUploadProgress> file_transfers;

  std::jthread eventThread {&ConnectionManager::runEventLoop, &client};
  int fd {client.connect(argv[1], std::stoi(argv[2]))};

  if (fd < 0)
  {
    std::cerr << "Failed to connect." << std::endl;
    client.stopEventLoop();
    return 1;
  }
  
  for (int i = 3; i < argc; ++i)
  {
    {
      std::jthread uploadThread {
        [&i, &fd, &client, argv]()
        {
          std::string file_name {argv[i]};
          std::ifstream input_file_stream {file_name, std::ios::in | std::ios::binary | std::ios::ate};
          if (!input_file_stream.is_open())
          {
            std::cerr << std::strerror(errno) << std::endl;
            return;
          }
          std::uint64_t file_size = input_file_stream.tellg();
          input_file_stream.close();
          std::cout << file_size << std::endl;
          FileUploadRequestHeader file_upload_request_header;
          file_upload_request_header.filename_size = file_name.size();
          file_upload_request_header.file_size = file_size;
          std::vector<char> payload (sizeof(FileUploadRequestHeader) + file_name.size());
          std::memcpy(payload.data(), &file_upload_request_header, sizeof(FileUploadRequestHeader));
          std::memcpy(payload.data()+sizeof(FileUploadRequestHeader), file_name.data(), file_name.size());
          if (client.send(fd, MessageType::FILE_UPLOAD_REQUEST, payload.data(), payload.size()) < 0)
            std::cerr << "Upload file request failed." << std::endl;
        }
      };
    }
  }

  while (!shutdown_requested)
  {
    std::cout << "\r\033[2K";
    for (const auto& [key, value]: file_uploads)
    {
      std::cout << key
                << ": "
                << ((static_cast<double>(value.bytes_sent) / static_cast<double>(value.file_size)) * 100)
                << "%; ";
    }
    std::cout << std::fixed << std::setprecision(2) << std::flush;
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }

  client.stopEventLoop();
  client.disconnect(fd);

  return 0;
}
