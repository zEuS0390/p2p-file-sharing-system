#ifndef CORE_TYPES_EVENT_COMMAND_HPP
#define CORE_TYPES_EVENT_COMMAND_HPP

#include <vector>

#include "core/types/CommandType.hpp"

struct EventCommand
{
  CommandType m_type;
  int m_socket_descriptor;
  std::vector<char> m_payload;
  EventCommand(
    const CommandType& type,
    int socket_descriptor = -1,
    const std::vector<char>& payload = {}
  ):
    m_type{type},
    m_socket_descriptor{socket_descriptor},
    m_payload{payload}
  {
  }
};

#endif
