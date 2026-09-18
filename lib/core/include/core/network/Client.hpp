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
  explicit Client(IMessageHandler&);
  ~Client();
  int connect(const std::string&, int);
  int disconnect(int);
  int send(int, const MessageType&, const char*, size_t);
  void start();
  void stop();
};

#endif

