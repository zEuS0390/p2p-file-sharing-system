#ifndef CORE_NETWORK_INTERFACE_MESSAGE_HANDLER
#define CORE_NETWORK_INTERFACE_MESSAGE_HANDLER

#include "core/types/Connection.hpp"

/*
 * Interface class for both client and server to manage message operations.
 */
class IMessageHandler
{
public:
  virtual void dispatchMessage(
    Connection& connection,
    const char* data
  ) = 0;
  virtual void queueMessage(
    Connection& connection,
    const MessageType& message_type,
    const char* data,
    size_t data_size
  ) = 0;
  virtual bool queueNextFileChunk(
    Connection& connection,
    std::uint64_t transfer_id
  ) = 0;
  virtual ~IMessageHandler() = default;
};

#endif // CORE_NETWORK_INTERFACE_MESSAGE_HANDLER

