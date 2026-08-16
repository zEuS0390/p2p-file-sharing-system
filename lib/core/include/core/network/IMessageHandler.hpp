#ifndef CORE_NETWORK_INTERFACE_MESSAGE_HANDLER
#define CORE_NETWORK_INTERFACE_MESSAGE_HANDLER

#include <memory>

#include "core/types/MessageHeaders.hpp"
#include "core/types/Connection.hpp"

/*
 * Interface class for both client and server to manage message operations.
 */
class IMessageHandler
{
public:
  virtual void dispatchMessage(
    std::shared_ptr<Connection>,
    MessageHeader&,
    const char*
  ) = 0;
  virtual void queueMessage(
    std::shared_ptr<Connection>,
    const MessageType&,
    const char*,
    size_t
  ) = 0;
  virtual ~IMessageHandler() = default;
};

#endif
