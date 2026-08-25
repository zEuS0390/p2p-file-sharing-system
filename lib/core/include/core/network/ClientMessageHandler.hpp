#ifndef CORE_NETWORK_CLIENT_MESSAGE_HANDLER_HPP
#define CORE_NETWORK_CLIENT_MESSAGE_HANDLER_HPP

#include <memory>

#include "core/network/IMessageHandler.hpp"
#include "core/types/Connection.hpp"
#include <memory>

/*
 * Handles message operations for the ClientConnectionManager.
 */
class ClientMessageHandler: public IMessageHandler
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

