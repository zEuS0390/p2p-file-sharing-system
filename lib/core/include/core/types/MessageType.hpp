#ifndef CORE_TYPES_MESSAGE_TYPE_HPP
#define CORE_TYPES_MESSAGE_TYPE_HPP

#include <cstdint>

enum class MessageType: std::uint8_t
{
  FILE_DOWNLOAD_REQUEST,
  FILE_REQUEST,
  FILE_INFO,
  FILE_CHUNK,
  FILE_ACK,
  FILE_END,
  FILE_ERROR,
  MESSAGE
};

#endif

