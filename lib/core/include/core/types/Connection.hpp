#ifndef CORE_TYPES_CONNECTION_HPP
#define CORE_TYPES_CONNECTION_HPP

#include <mutex>
#include <vector>
#include <poll.h>
#include <atomic>

#include "core/types/Endpoint.hpp"
#include "core/types/MessageHeaders.hpp"

/*
 * A structure for both server and client containing connection information.
 */
struct Connection 
{
  Endpoint endpoint {};
  std::atomic<int> socket_descriptor {-1};
  std::vector<char> recv_buffer;
  bool reading_header {true};
  size_t recv_offset {0};
  std::vector<char> send_buffer;
  size_t send_offset {0};
  MessageHeader current_header {};
  std::mutex mutex;
};

#endif // CORE_TYPES_CONNECTION_HPP

