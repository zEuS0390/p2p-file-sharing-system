#ifndef CORE_NETWORK_SERVER_MESSAGE_HANDLER_HPP
#define CORE_NETWORK_SERVER_MESSAGE_HANDLER_HPP

#include <memory>

#include "core/network/IMessageHandler.hpp"
#include "core/types/Connection.hpp"

/*
 * Handles message operations for the ServerConnectionManager.
 */
class ServerMessageHandler: public IMessageHandler
{
public:
  void dispatchMessage(
    std::shared_ptr<Connection>,
    MessageHeader&,
    const char*
  ) override;
  void queueMessage(
    std::shared_ptr<Connection>,
    const MessageType&,
    const char* data,
    size_t
  ) override;
};

#endif

