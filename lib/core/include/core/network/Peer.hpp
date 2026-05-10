#ifndef NETWORK_PEER_HPP
#define NETWORK_PEER_HPP

#include <thread>

#include "core/network/Client.hpp"
#include "core/network/Server.hpp"

class Peer: public Server, public Client
{
private:
  std::thread listening_thread;
  std::thread monitoring_thread;
public:
  Peer();
  ~Peer();
  void start();
  void stop();
};

#endif

