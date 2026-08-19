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
Server::Server(IMessageHandler& message_handler):
  m_server_connection_manager(message_handler)
{
}

// Destructor
Server::~Server()
{
  if (is_running)
    stop();
}

// Listen for incoming client connections
void Server::start(uint16_t port)
{
  m_server_connection_manager.initServer(port);
  accept_thread = std::thread{&ServerConnectionManager::startAcceptConnectionLoop, std::ref(m_server_connection_manager)};
  event_thread = std::thread{&ServerConnectionManager::runEventLoop, std::ref(m_server_connection_manager)};
}

// Stop listening for incoming client conncections
void Server::stop()
{
  m_server_connection_manager.stopAcceptConnectionLoop();
  m_server_connection_manager.stopEventLoop();
  if (accept_thread.joinable())
    accept_thread.join();
  if (event_thread.joinable())
    event_thread.join();
}

