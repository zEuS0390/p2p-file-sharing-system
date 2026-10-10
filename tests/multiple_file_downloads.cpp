#include "core/network/ConnectionManager.hpp"
#include "core/network/MessageHandler.hpp"
#include "core/types/MessageHeaders.hpp"
#include "core/types/MessageType.hpp"
#include <chrono>
#include <csignal>
#include <cstring>
#include <string>
#include <thread>
#include <cstdint>
#include <unordered_map>

volatile sig_atomic_t shutdown_requested = 0;

void handle_signal(int)
{
  shutdown_requested = 1;
}

struct FileDownloadProgress
{
  std::uint64_t file_size {0};
  std::uint64_t bytes_received {0};
};

int main(int argc, const char** argv)
{
  if (argc < 4)
  {
    std::cerr << "Usage: "
              << argv[0]
              << ": <hostname> <port> <filename1> <filename2> ..."
              << std::endl;
    return 1;
  }

  std::signal(SIGTERM, handle_signal);
  std::signal(SIGINT, handle_signal);

  MessageHandler message_handler;
  ConnectionManager client {message_handler};

  std::unordered_map<std::uint64_t, FileDownloadProgress> file_download_progress;

  message_handler.on<MessageType::FILE_DOWNLOAD_RESPONSE>(
    [&file_download_progress](std::uint64_t transfer_id, std::uint64_t file_size)
    {
      std::cout << "Downloading request " << transfer_id << " ...\n" << std::flush;
      file_download_progress.emplace(transfer_id, FileDownloadProgress{file_size,0});
    }
  );

  message_handler.on<MessageType::FILE_CHUNK>(
    [&file_download_progress](std::uint64_t transfer_id, std::uint64_t chunk_size)
    {
      auto it {file_download_progress.find(transfer_id)};
      if (it == file_download_progress.end())
        return;
      file_download_progress[transfer_id].bytes_received += chunk_size;
      if (file_download_progress[transfer_id].bytes_received == file_download_progress[transfer_id].file_size)
        file_download_progress.erase(transfer_id);
    }
  );

  std::jthread evenThread {&ConnectionManager::runEventLoop, &client};

  int fd {client.connect(argv[1], std::stoi(argv[2]))};

  if (fd < 0)
  {
    std::cerr << "Failed to connect." << std::endl;
    return 1;
  }

  for (int i = 3; i < argc; ++i)
  {
    {
      std::jthread sendThread {
        [&fd, &client, &i, &argv]()
        {
          std::string file_name_str {argv[i]};
          FileDownloadRequestHeader file_download_request {};
          file_download_request.request_id = 2,
          file_download_request.filename_size = file_name_str.size();
          std::vector<char> payload (sizeof(FileDownloadRequestHeader) + file_name_str.size());
          std::memcpy(payload.data(), &file_download_request, sizeof(FileDownloadRequestHeader));
          std::memcpy(payload.data()+sizeof(FileDownloadRequestHeader), file_name_str.data(), file_name_str.size());
          if (client.send(fd, MessageType::FILE_DOWNLOAD_REQUEST, payload.data(), payload.size()) < 0)
            std::cerr << "Unable to download the file." << std::endl;
        }
      };
    }
  }

  while (!shutdown_requested)
  {
    std::cout << "\r\033[2K";
    for (const auto& [key, value]: file_download_progress)
    {
      std::cout << key
                << ": "
                << ((static_cast<double>(value.bytes_received) / static_cast<double>(value.file_size)) * 100)
                << "%; ";
    }
    std::cout << std::fixed << std::setprecision(2) << std::flush;
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }

  client.disconnect(fd);
  client.stopEventLoop();
}
