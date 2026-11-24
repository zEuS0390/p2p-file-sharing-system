#ifndef P2P_FSS_TYPES_OUTGOING_FILE_TRANSFER_HPP
#define P2P_FSS_TYPES_OUTGOING_FILE_TRANSFER_HPP

#include <cstdint>
#include <fstream>

struct OutgoingFileTransfer
{
  std::ifstream file;
  std::string filename;
  std::uint64_t file_size {};
  std::uint64_t bytes_sent {};
};

#endif // P2P_FSS_TYPES_OUTGOING_FILE_TRANSFER_HPP

