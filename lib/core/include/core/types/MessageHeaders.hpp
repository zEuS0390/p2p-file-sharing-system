#ifndef CORE_TYPES_MESSAGE_HEADER_HPP
#define CORE_TYPES_MESSAGE_HEADER_HPP

#include <cstdint>

#include "core/types/MessageType.hpp"
#include "core/types/FileErrorCode.hpp"

struct MessageHeader
{
  MessageType type;
  std::uint32_t payload_size {};
};

struct MessageRequestHeader
{
  std::uint64_t request_id {};
  std::uint64_t message_size {};
};

struct MessageResponseHeader 
{
  std::uint64_t request_id {};
  std::uint64_t message_size {};
};

struct FileDownloadRequestHeader
{
  std::uint64_t request_id {};
  std::uint32_t filename_size {};
};

struct FileDownloadResponseHeader
{
  std::uint64_t request_id {};
  std::uint64_t transfer_id {};
  std::uint32_t filename_size {};
  std::uint64_t file_size {};
};

struct FileUploadRequestHeader
{
  std::uint32_t filename_size {};
  std::uint64_t file_size {};
};

struct FileUploadResponseHeader 
{
  std::uint64_t transfer_id {};
  std::uint32_t filename_size {};
};

struct FileRequestHeader
{
  std::uint32_t filename_size {};
};

struct FileChunkHeader
{
  std::uint64_t transfer_id {};
  std::uint32_t chunk_size {};
};

struct FileEndHeader 
{
  std::uint64_t transfer_id {};
};

struct FileErrorHeader
{
  FileErrorCode error_code;
  std::uint32_t message_size {};
};

#endif // CORE_TYPES_MESSAGE_HEADER_HPP

