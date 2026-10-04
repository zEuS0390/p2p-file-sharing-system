#ifndef CORE_TYPES_EVENT_COMMAND_HPP
#define CORE_TYPES_EVENT_COMMAND_HPP

#include <future>
#include <unordered_set>
#include <variant>
#include <vector>

#include "core/types/Endpoint.hpp"
#include "core/types/MessageType.hpp"

struct AddAllListenerSocketsEventCommand
{
  std::unordered_set<int> m_listen_fds;
};

struct AddConnectionEventCommand
{
  int m_socket_descriptor {-1};
  Endpoint endpoint {};
};

struct RemoveConnectionEventCommand
{
  int m_socket_descriptor {-1};
};

struct SendMessageEventCommand
{
  int m_socket_descriptor {-1};
  MessageType m_message_type {};
  std::vector<char> m_payload;
  std::promise<int> result;
};

using EventCommand = std::variant<
  AddAllListenerSocketsEventCommand,
  AddConnectionEventCommand,
  RemoveConnectionEventCommand,
  SendMessageEventCommand
>;

#endif // CORE_TYPES_EVENT_COMMAND_HPP

