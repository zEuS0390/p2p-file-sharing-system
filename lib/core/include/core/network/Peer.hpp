#ifndef NETWORK_PEER_HPP
#define NETWORK_PEER_HPP

#include <cstdint>
#include <string>
#include <thread>

#include "core/network/Client.hpp"
#include "core/network/IMessageHandler.hpp"
#include "core/network/Server.hpp"
#include "core/types/MessageType.hpp"

class Peer
{
private:
  Client client;
  Server server;
private:
  std::thread server_thread;
  std::thread client_thread;
  bool is_running;
public:
  Peer(IMessageHandler&, IMessageHandler&);
  ~Peer();
  void start(uint16_t);
  void stop();
  int connect(const std::string&, int);
  int disconnect(int);
  ssize_t send(int, const MessageType&, const char*, size_t);
};

#endif

