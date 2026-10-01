#ifndef CORE_NETWORK_SERVER_MESSAGE_HANDLER_HPP
#define CORE_NETWORK_SERVER_MESSAGE_HANDLER_HPP

#include "core/network/IMessageHandler.hpp"
#include "core/types/Connection.hpp"

/*
 * Handles message operations for the ServerConnectionManager.
 */
class ServerMessageHandler: public IMessageHandler
{
public:
  void dispatchMessage(
    Connection& connection,
    const char* data
  ) override;
  void queueMessage(
    Connection& connection_type,
    const MessageType& message_type,
    const char* data,
    size_t data_size
  ) override;
};

#endif // CORE_NETWORK_SERVER_MESSAGE_HANDLER_HPP

