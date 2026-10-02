#ifndef CORE_NETWORK_CLIENT_HPP
#define CORE_NETWORK_CLIENT_HPP

#include <string>
#include <thread>

#include "core/network/ClientConnectionManager.hpp"
#include "core/network/IMessageHandler.hpp"
#include "core/types/MessageType.hpp"

/*
 * Manages the connection to the server and provides
 * operations for communicating with it.
 */
class Client
{
private:
  ClientConnectionManager m_client_connection_manager;
  std::thread m_event_thread;
public:
  explicit Client(IMessageHandler& message_handler) noexcept;
  ~Client();
  int connect(const std::string& hostname, int port);
  int disconnect(int socket_descriptor);
  int send(
    int socket_descriptor,
    const MessageType& message_type,
    const char* data,
    size_t length
  );
  void start();
  void stop();
};

#endif // CORE_NETWORK_CLIENT_HPP

