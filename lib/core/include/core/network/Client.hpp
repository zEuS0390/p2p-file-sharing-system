#ifndef CORE_NETWORK_CLIENT_HPP
#define CORE_NETWORK_CLIENT_HPP

#include <string>
#include <thread>

#include "core/network/ClientConnectionManager.hpp"
#include "core/network/IMessageHandler.hpp"

class Client
{
private:
  ClientConnectionManager client_connection_manager;
  std::thread event_thread;
public:
  Client(IMessageHandler&);
  ~Client();
  int connectToServer(const std::string&, int);
  int disconnectToServer(int);
  ssize_t sendAll(int, const char*, size_t);
  void start();
  void stop();
};

#endif
