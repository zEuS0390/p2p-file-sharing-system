#ifndef P2P_FSS_NETWORK_CONNECTION_MANAGER_HPP
#define P2P_FSS_NETWORK_CONNECTION_MANAGER_HPP

#include <unordered_map>
#include <unordered_set>
#include <sys/epoll.h>
#include <memory>
#include <atomic>
#include <string>
#include <mutex>
#include <queue>

#include "p2p_fss/network/MessageHandler.hpp"
#include "p2p_fss/network/IOMultiplexer.hpp"
#include "p2p_fss/types/MessageType.hpp"
#include "p2p_fss/types/Connection.hpp"
#include "p2p_fss/types/Command.hpp"

class ConnectionManager
{
private:
  using Commands = std::queue<Command::Type>;
  using Connections = std::unordered_map<int, std::unique_ptr<Connection> >;
  void processCommand(Command::AddAllListenerSockets& command);
  void processCommand(Command::AddConnection& command);
  void processCommand(Command::RemoveConnection& command);
  void processCommand(Command::SendMessage& command);
  void processCommands();
  void parseIncomingMessage(Connection& connection);
public:
  MessageHandler m_message_handler;
  ConnectionManager();
  ~ConnectionManager();
  void initListeners(std::uint16_t port);
  void startListening();
  void stopListening();
  void runEventLoop();
  void stopEventLoop();
  bool getEventLoopStatus() const;
  void setEventLoopStatus(bool status);
  int connect(const std::string& hostname, int port);
  int disconnect(int socket_descriptor);
  int send(
    int socket_descriptor,
    const MessageType& message_type,
    const char* data,
    std::size_t length
  );
protected:
  IOMultiplexer             m_io_multiplexer;
  std::mutex                m_mutex;
  std::atomic<bool>         m_is_event_running;
  std::atomic<bool>         m_is_listen_running;
  std::atomic<int>          m_command_fd;
  std::unordered_set<int>   m_listen_fds;
  Commands                  m_commands;
  Connections               m_connections;
};

#endif // P2P_FSS_NETWORK_CONNECTION_MANAGER_HPP

