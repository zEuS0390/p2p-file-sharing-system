#ifndef CORE_NETWORK_CONNECTION_MANAGER_HPP
#define CORE_NETWORK_CONNECTION_MANAGER_HPP

#include <unordered_map>
#include <memory>
#include <atomic>
#include <vector>
#include <poll.h>
#include <mutex>

#include "core/network/IMessageHandler.hpp"
#include "core/types/Connection.hpp"

class ConnectionManager
{
protected:
  int descriptor;
  std::unordered_map<int, std::shared_ptr<Connection>> connections;
  std::vector<pollfd> connection_pollfds;
private:
  std::atomic<bool> is_event_running;
  IMessageHandler& message_handler;
public:
  std::mutex mutex;
  ConnectionManager(IMessageHandler&);
  ~ConnectionManager();
  void runEventLoop();
  void stopEventLoop();
  void parseIncomingMessage(std::shared_ptr<Connection>);
  void addConnection(int, std::shared_ptr<Connection>, pollfd);
  void removeConnection(int);
  ssize_t sendAll(int, const char*, size_t);
};

#endif
