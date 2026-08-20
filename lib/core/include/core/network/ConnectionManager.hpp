#ifndef CORE_NETWORK_CONNECTION_MANAGER_HPP
#define CORE_NETWORK_CONNECTION_MANAGER_HPP

#include <unordered_map>
#include <memory>
#include <atomic>
#include <poll.h>
#include <mutex>

#include "core/network/IMessageHandler.hpp"
#include "core/types/Connection.hpp"
#include "core/types/MessageType.hpp"

/*
 * Base class for both client and server to manage connections.
 */
class ConnectionManager
{
protected:
  IMessageHandler& m_message_handler;
  std::mutex m_mutex;
  std::atomic<bool> m_is_event_running;
  std::unordered_map<int, std::shared_ptr<Connection>> m_connections;
public:
  explicit ConnectionManager(IMessageHandler&);
  ~ConnectionManager();
  void runEventLoop();
  void stopEventLoop();
  void parseIncomingMessage(std::shared_ptr<Connection>);
  void addConnection(int, const Endpoint&);
  void removeConnection(int);
  ssize_t send(int, const MessageType&, const char*, size_t);
};

#endif

