#ifndef NETWORK_CLIENT_CONNECTION_HPP
#define NETWORK_CLIENT_CONNECTION_HPP

#include <vector>

#include "core/types/Endpoint.hpp"
#include "core/types/MessageHeaders.hpp"

struct ClientConnection
{
  Endpoint endpoint;
  std::vector<char> recv_buffer;
  bool reading_header {true};
  MessageHeader current_header {};
};

#endif
