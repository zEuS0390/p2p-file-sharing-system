#ifndef CORE_TYPES_INCOMING_FILE_TRANSFER_HPP
#define CORE_TYPES_INCOMING_FILE_TRANSFER_HPP

#include <cstdint>
#include <fstream>

struct IncomingFileTransfer
{
  std::ofstream file;
  std::string filename;
  std::uint64_t file_size {0};
  std::uint64_t bytes_received {0};
};

#endif // CORE_TYPES_INCOMING_FILE_TRANSFER_HPP

