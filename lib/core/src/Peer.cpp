#include <thread>

#include "core/network/Peer.hpp"
#include "core/network/IMessageHandler.hpp"

Peer::Peer(
  IMessageHandler& server_message_handler,
  IMessageHandler& client_message_handler):
  m_client(client_message_handler),
  m_server(server_message_handler),
  m_is_running{false}
{
}

Peer::~Peer()
{
  if (m_is_running)
    stop();
  if (m_server_thread.joinable())
    m_server_thread.join();
  if (m_client_thread.joinable())
    m_client_thread.join();
}

void Peer::start(uint16_t port)
{
  m_is_running = true;
  m_server_thread = std::thread {&Server::start, std::ref(m_server), port};
  m_client_thread = std::thread {&Client::start, std::ref(m_client)};
}

void Peer::stop()
{
  m_is_running = false;
  m_server.stop();
  m_client.stop();
}

int Peer::connect(const std::string& hostname, int port)
{
  return m_client.connect(hostname, port);
}

int Peer::disconnect(int socket_descriptor)
{
  return m_client.disconnect(socket_descriptor);
}

ssize_t Peer::send(
  int socket_descriptor,
  const MessageType& message_type,
  const char* data,
  size_t length
)
{
  return m_client.send(socket_descriptor, message_type, data, length);
}

