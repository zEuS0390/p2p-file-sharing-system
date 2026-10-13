#ifndef CORE_TYPES_COMMAND_HPP
#define CORE_TYPES_COMMAND_HPP

#include <future>
#include <unordered_set>
#include <variant>
#include <vector>

#include "core/types/Endpoint.hpp"
#include "core/types/MessageType.hpp"

namespace Command
{
  struct AddAllListenerSockets
  {
    std::unordered_set<int> m_listen_fds;
  };

  struct AddConnection
  {
    int m_socket_descriptor {-1};
    Endpoint endpoint {};
  };

  struct RemoveConnection
  {
    int m_socket_descriptor {-1};
  };

  struct SendMessage
  {
    int m_socket_descriptor {-1};
    MessageType m_message_type {};
    std::vector<char> m_payload;
    std::promise<int> result;
  };

  using Type = std::variant<
    AddAllListenerSockets,
    AddConnection,
    RemoveConnection,
    SendMessage
  >;
}

#endif // CORE_TYPES_EVENT_COMMAND_HPP

