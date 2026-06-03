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

class ConnectionManager
{
protected:
  int descriptor;
  std::mutex mutex;
  std::unordered_map<int, std::shared_ptr<Connection>> connections;
  std::unordered_map<int, std::shared_ptr<pollfd>> connection_pollfds;
private:
  std::atomic<bool> is_event_running;
  IMessageHandler& message_handler;
public:
  ConnectionManager(IMessageHandler&);
  ~ConnectionManager();
  void runEventLoop();
  void stopEventLoop();
  void parseIncomingMessage(std::shared_ptr<Connection>, std::shared_ptr<pollfd>);
  void addConnection(int, std::shared_ptr<Connection>, pollfd);
  void removeConnection(int);
  ssize_t sendAll(int, const MessageType&, const char*, size_t);
};

#endif
