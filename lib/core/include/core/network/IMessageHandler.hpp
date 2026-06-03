#ifndef CORE_NETWORK_INTERFACE_MESSAGE_HANDLER
#define CORE_NETWORK_INTERFACE_MESSAGE_HANDLER

#include <memory>

#include "core/types/MessageHeaders.hpp"
#include "core/types/Connection.hpp"

class IMessageHandler
{
public:
  virtual void dispatchMessage(
    std::shared_ptr<Connection>,
    std::shared_ptr<pollfd>,
    MessageHeader&,
    const char*
  ) = 0;
  virtual void queueMessage(
    std::shared_ptr<Connection>,
    std::shared_ptr<pollfd>,
    const MessageType&,
    const char*,
    size_t
  ) = 0;
  virtual ~IMessageHandler() = default;
};

#endif
