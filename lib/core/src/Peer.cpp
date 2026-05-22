#include "core/network/Peer.hpp"

Peer::Peer()
{
}

Peer::~Peer()
{
  stop();
  if (server_listening_thread.joinable())
    server_listening_thread.join();
  if (server_event_loop_thread.joinable())
    server_event_loop_thread.join();
}

void Peer::start()
{
  server_listening_thread = std::thread{&Peer::Server::startListening, this};
  server_event_loop_thread = std::thread{&Peer::Server::runEventLoop, this};
}

void Peer::stop()
{
  Server::stopListening();
  Server::stopEventLoop();
}

