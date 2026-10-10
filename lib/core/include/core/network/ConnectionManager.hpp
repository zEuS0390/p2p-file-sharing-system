#ifndef CORE_NETWORK_CONNECTION_MANAGER_HPP
#define CORE_NETWORK_CONNECTION_MANAGER_HPP

#include <unordered_map>
#include <sys/epoll.h>
#include <cstdint>
#include <memory>
#include <atomic>
#include <mutex>
#include <queue>
#include <string>
#include <unordered_set>

#include "core/network/IOMultiplexer.hpp"
#include "core/network/MessageHandler.hpp"
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
  MessageHandler& m_message_handler;
  IOMultiplexer m_io_multiplexer;
  std::mutex m_mutex;
  std::atomic<bool> m_is_listen_running;
  std::atomic<int> m_command_fd;
  std::unordered_set<int> m_listen_fds;
  std::queue<EventCommand> m_event_commands;
  std::unordered_map<int, std::unique_ptr<Connection> > m_connections;
private:
  void processCommand(AddAllListenerSocketsEventCommand& event_command);
  void processCommand(AddConnectionEventCommand& event_command);
  void processCommand(RemoveConnectionEventCommand& event_command);
  void processCommand(SendMessageEventCommand& event_command);
  void processCommands();
  void parseIncomingMessage(Connection& connection);
public:
  std::atomic<bool> m_is_event_running;
  explicit ConnectionManager(MessageHandler& message_handler);
  ~ConnectionManager();
  void initListeners(std::uint16_t port);
  void startListening();
  void stopListening();
  void runEventLoop();
  void stopEventLoop();
  void addConnection(
    int socket_descriptor,
    const Endpoint& endpoint
  );
  void removeConnection(int socket_descriptor);
  int connect(const std::string& hostname, int port);
  int disconnect(int socket_descriptor);
  int send(
    int socket_descriptor,
    const MessageType& message_type,
    const char* data,
    size_t length
  );
};

#endif // CORE_NETWORK_CONNECTION_MANAGER_HPP

