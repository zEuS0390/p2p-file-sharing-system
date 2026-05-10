#include "core/network/Peer.hpp"

Peer::Peer()
{
}

Peer::~Peer()
{
  stop();
  if (listening_thread.joinable())
    listening_thread.join();
  if (monitoring_thread.joinable())
    monitoring_thread.join();
}

void Peer::start()
{
  listening_thread = std::thread{&Peer::startListening, this};
  monitoring_thread = std::thread{&Peer::startMonitoring, this};
}

void Peer::stop()
{
  stopListening();
  stopMonitoring();
}

