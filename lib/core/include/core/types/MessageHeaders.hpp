#ifndef NETWORK_MESSAGE_HEADER_HPP
#define NETWORK_MESSAGE_HEADER_HPP

#include <cstdint>

#include "core/types/MessageType.hpp"

struct MessageHeader
{
  MessageType type;
  uint32_t payload_size {};
};

struct FileInfoHeader
{
  uint32_t filename_size {};
  uint64_t file_size {};
};

#endif

