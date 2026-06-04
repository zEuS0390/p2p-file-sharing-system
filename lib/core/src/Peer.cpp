#include <thread>

#include "core/network/Peer.hpp"
#include "core/network/IMessageHandler.hpp"

Peer::Peer(
  IMessageHandler& server_message_handler,
  IMessageHandler& client_message_handler):
  client(client_message_handler),
  server(server_message_handler),
  is_running{false}
{
}

Peer::~Peer()
{
  if (is_running)
    stop();
  if (server_thread.joinable())
    server_thread.join();
  if (client_thread.joinable())
    client_thread.join();
}

void Peer::start(uint16_t port)
{
  is_running = true;
  server_thread = std::thread {&Server::start, std::ref(server), port};
  client_thread = std::thread {&Client::start, std::ref(client)};
}

void Peer::stop()
{
  is_running = false;
  server.stop();
  client.stop();
}

int Peer::connect(const std::string& hostname, int port)
{
  return client.connect(hostname, port);
}

int Peer::disconnect(int socket_descriptor)
{
  return client.disconnect(socket_descriptor);
}

ssize_t Peer::send(
  int socket_descriptor,
  const MessageType& message_type,
  const char* data,
  size_t length
)
{
  return client.send(socket_descriptor, message_type, data, length);
}
