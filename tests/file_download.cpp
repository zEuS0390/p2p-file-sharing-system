#include <cstring>
#include <iomanip>
#include <iostream>
#include <string>
#include <string_view>
#include <thread>
#include <unordered_map>

#include "core/network/ConnectionManager.hpp"
#include "core/network/MessageHandler.hpp"
#include "core/types/MessageHeaders.hpp"

// Main Entry Point of the Program
int main(int argc, const char* argv[])
{
  if (argc != 4)
  {
    std::cerr << "Usage: "
              << argv[0]
              << " <hostname> <port> <filename>"
              << std::endl;
    return 1;
  }

  MessageHandler message_handler;
  ConnectionManager client {message_handler};

  std::unordered_map<std::uint64_t, std::vector<std::uint64_t> > file_transfer;

  message_handler.on<MessageType::FILE_DOWNLOAD_RESPONSE>(
    [&file_transfer](
      std::uint64_t transfer_id,
      std::uint64_t file_size
    )
    {
      file_transfer.emplace(transfer_id, std::vector<std::uint64_t>{file_size, 0});
    }
  );

  message_handler.on<MessageType::FILE_CHUNK>(
    [&file_transfer](
      std::uint64_t transfer_id,
      std::uint64_t chunk_size
    )
    {
      auto it {file_transfer.find(transfer_id)};
      if (it != file_transfer.end())
      {
        auto& progress {it->second};
        progress.at(1) += chunk_size;
        const double bytes_received = progress.at(1);
        const double file_size = progress.at(0);
        std::cout << "\r\033[2K"
                  << bytes_received
                  << "/"
                  << file_size
                  << " "
                  << (bytes_received / file_size) * 100
                  << "%"
                  << std::fixed
                  << std::setprecision(1)
                  << std::flush;
        if (bytes_received == file_size)
        {
          std::cout << "\nFile download complete.\n" << std::flush;
          file_transfer.erase(it);
        }
      }
    }
  );

  int socket_descriptor {client.connect(argv[1], std::stoi(argv[2]))};

  std::thread eventThread {&ConnectionManager::runEventLoop, &client};

  std::string file_name {argv[3]};
  FileDownloadRequestHeader file_download_request_header {};
  file_download_request_header.filename_size = file_name.size();
  file_download_request_header.request_id = 1;

  std::vector<char> bytes {};
  bytes.resize(file_name.size() + sizeof(FileDownloadRequestHeader));
  std::memcpy(bytes.data(), &file_download_request_header, sizeof(FileDownloadRequestHeader));
  std::memcpy(bytes.data()+sizeof(FileDownloadRequestHeader), file_name.data(), file_name.size());

  if (client.send(socket_descriptor, MessageType::FILE_DOWNLOAD_REQUEST, bytes.data(), bytes.size()) < 0)
  {
    std::cerr << strerror(errno) << std::endl;
    return 1;
  }
  
  std::cout << "Press enter to stop ...";
  std::cin.get();

  client.disconnect(socket_descriptor);

  client.stopEventLoop();

  eventThread.join();

  return 0;
}

