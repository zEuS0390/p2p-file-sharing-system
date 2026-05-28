#ifndef CORE_NETWORK_SERVER_HPP
#define CORE_NETWORK_SERVER_HPP

#include <sys/socket.h>
#include <thread>
#include <poll.h>

#include "core/network/IMessageHandler.hpp"
#include "core/network/ServerConnectionManager.hpp"

class Server
{
private:
  ServerConnectionManager server_connection_manager;
  std::thread accept_thread;
  std::thread event_thread;
public:
  Server(IMessageHandler&);
  ~Server();
  void start();
  void stop();
};

#endif

