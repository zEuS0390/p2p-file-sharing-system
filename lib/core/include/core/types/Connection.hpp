#ifndef CORE_TYPES_CONNECTION_HPP
#define CORE_TYPES_CONNECTION_HPP

#include <mutex>
#include <vector>

#include "core/types/Endpoint.hpp"
#include "core/types/MessageHeaders.hpp"

struct Connection 
{
  Endpoint endpoint;
  std::vector<char> recv_buffer;
  bool reading_header {true};
  size_t recv_offset {0};
  std::vector<char> send_buffer;
  size_t send_offset {0};
  MessageHeader current_header {};
  std::mutex mutex;
};

#endif
