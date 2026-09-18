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
  Client m_client;
  Server m_server;
private:
  std::thread m_server_thread;
  std::thread m_client_thread;
  bool m_is_running;
public:
  explicit Peer(IMessageHandler&, IMessageHandler&);
  ~Peer();
  void start(uint16_t);
  void stop();
  int connect(const std::string&, int);
  int disconnect(int);
  int send(int, const MessageType&, const char*, size_t);
};

#endif

