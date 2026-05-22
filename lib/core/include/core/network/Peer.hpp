#ifndef NETWORK_PEER_HPP
#define NETWORK_PEER_HPP

#include <thread>

#include "core/network/Client.hpp"
#include "core/network/Server.hpp"

class Peer: public Server, public Client
{
private:
  std::thread server_listening_thread;
  std::thread server_event_loop_thread;
public:
  Peer();
  ~Peer();
  void start();
  void stop();
};

#endif

