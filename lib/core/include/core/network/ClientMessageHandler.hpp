#ifndef CORE_NETWORK_CLIENT_MESSAGE_HANDLER_HPP
#define CORE_NETWORK_CLIENT_MESSAGE_HANDLER_HPP

#include <memory>

#include "core/network/IMessageHandler.hpp"
#include "core/types/Connection.hpp"
#include <memory>

class ClientMessageHandler: public IMessageHandler
{
public:
  virtual void dispatchMessage(std::shared_ptr<Connection>, MessageHeader&, const char*) override;
  virtual void queueMessage(std::shared_ptr<Connection>, const MessageType&, const char* data, size_t) override;
};

#endif

