#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>
#include <cstring>
#include <cerrno>

#include "core/network/Server.hpp"
#include "core/network/IMessageHandler.hpp"
#include "core/network/ServerConnectionManager.hpp"

// Constructor
Server::Server(IMessageHandler& message_handler) noexcept:
  m_server_connection_manager(message_handler)
{
}

// Destructor
Server::~Server()
{
  if (m_is_running)
    stop();
}

// Listen for incoming client connections
void Server::start(uint16_t port)
{
  m_is_running = true;
  m_server_connection_manager.initServer(port);
  m_accept_thread = std::thread{&ServerConnectionManager::startAcceptConnectionLoop, std::ref(m_server_connection_manager)};
  m_event_thread = std::thread{&ServerConnectionManager::runEventLoop, std::ref(m_server_connection_manager)};
}

// Stop listening for incoming client conncections
void Server::stop()
{
  m_is_running = false;
  m_server_connection_manager.stopAcceptConnectionLoop();
  m_server_connection_manager.stopEventLoop();
  if (m_accept_thread.joinable())
    m_accept_thread.join();
  if (m_event_thread.joinable())
    m_event_thread.join();
}

