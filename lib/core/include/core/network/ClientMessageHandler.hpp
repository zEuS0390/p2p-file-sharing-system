#ifndef CORE_NETWORK_CLIENT_MESSAGE_HANDLER_HPP
#define CORE_NETWORK_CLIENT_MESSAGE_HANDLER_HPP

#include "core/network/IMessageHandler.hpp"
#include "core/types/Connection.hpp"

/*
 * Handles message operations for the ClientConnectionManager.
 */
class ClientMessageHandler: public IMessageHandler
{
public:
  void dispatchMessage(
    Connection&,
    MessageHeader&,
    const char*
  ) override;
  void queueMessage(
    Connection&,
    const MessageType&,
    const char*,
    size_t
  ) override;
};

#endif // CORE_NETWORK_CLIENT_MESSAGE_HANDLER_HPP

