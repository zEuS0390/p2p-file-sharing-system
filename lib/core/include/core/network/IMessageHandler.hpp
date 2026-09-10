#ifndef CORE_NETWORK_INTERFACE_MESSAGE_HANDLER
#define CORE_NETWORK_INTERFACE_MESSAGE_HANDLER

#include "core/types/MessageHeaders.hpp"
#include "core/types/Connection.hpp"

/*
 * Interface class for both client and server to manage message operations.
 */
class IMessageHandler
{
public:
  virtual void dispatchMessage(
    Connection&,
    MessageHeader&,
    const char*
  ) = 0;
  virtual void queueMessage(
    Connection&,
    const MessageType&,
    const char*,
    size_t
  ) = 0;
  virtual ~IMessageHandler() = default;
};

#endif

