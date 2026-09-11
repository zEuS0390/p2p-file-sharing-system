#ifndef CORE_TYPES_EVENT_COMMAND_HPP
#define CORE_TYPES_EVENT_COMMAND_HPP

#include <variant>
#include <vector>

#include "core/types/Endpoint.hpp"
#include "core/types/MessageType.hpp"

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
};

using EventCommand = std::variant<
  AddConnectionEventCommand,
  RemoveConnectionEventCommand,
  SendMessageEventCommand
>;

#endif
