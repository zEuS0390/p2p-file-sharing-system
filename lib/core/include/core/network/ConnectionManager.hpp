#ifndef CORE_NETWORK_CONNECTION_MANAGER_HPP
#define CORE_NETWORK_CONNECTION_MANAGER_HPP

#include <unordered_map>
#include <sys/epoll.h>
#include <memory>
#include <atomic>
#include <mutex>
#include <array>
#include <queue>

#include "core/network/IMessageHandler.hpp"
#include "core/types/Connection.hpp"
#include "core/types/MessageType.hpp"
#include "core/types/EventCommand.hpp"

/*
 * Base class for both client and server to manage connections.
 */
class ConnectionManager
{
private:
  static constexpr int MAX_EVENTS {64};
protected:
  IMessageHandler& m_message_handler;
  std::mutex m_mutex;
  std::atomic<bool> m_is_event_running;
  std::atomic<int> m_epfd;
  std::atomic<int> m_command_fd;
  std::queue<EventCommand> m_event_commands;
  std::unordered_map<int, std::unique_ptr<Connection> > m_connections;
  std::array<epoll_event, MAX_EVENTS> m_epoll_events;
private:
  void processCommand(AddConnectionEventCommand&);
  void processCommand(RemoveConnectionEventCommand&);
  void processCommand(SendMessageEventCommand&);
  void processCommands();
  void updateEpollEvents(Connection&);
  void parseIncomingMessage(Connection&);
public:
  explicit ConnectionManager(IMessageHandler&);
  ~ConnectionManager();
  void runEventLoop();
  void stopEventLoop();
  void addConnection(int, const Endpoint&);
  void removeConnection(int);
  ssize_t send(int, const MessageType&, const char*, size_t);
};

#endif

