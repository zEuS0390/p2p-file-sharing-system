#include <iostream>
#include <mutex>
#include <netdb.h>
#include <stdexcept>
#include <cstring>
#include <sys/socket.h>
#include <unistd.h>
#include <sstream>

#include "core/network/ServerConnectionManager.hpp"
#include "core/network/IMessageHandler.hpp"

ServerConnectionManager::ServerConnectionManager(
  IMessageHandler& message_handler
):
  ConnectionManager{message_handler},
  is_listening{false}
{
}

void ServerConnectionManager::initServer(uint16_t port)
{
  // Enable to reuse the same address without the system restruction (ONLY FOR DEV and DEBUGGING) 
  int opt = 1;
  setsockopt(descriptor, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

  // // Create an address information for the server socket
  struct addrinfo* server_address {nullptr};
  struct addrinfo hints;
  std::memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_protocol = IPPROTO_TCP;
  hints.ai_flags = AI_PASSIVE;

  std::string port_str {std::to_string(port)};
  int getaddrinfo_status {getaddrinfo(nullptr, port_str.c_str(), &hints, &server_address)};

  if (getaddrinfo_status < 0)
    throw std::runtime_error(strerror(errno));

  // Bind the address information on the server socket
  int bind_result {
    bind(
      descriptor,
      server_address->ai_addr,
      server_address->ai_addrlen
    )
  };

  if (bind_result < 0)
    throw std::runtime_error(strerror(errno));

  int listen_status {listen(descriptor, SOMAXCONN)};

  if (listen_status < 0)
    throw std::runtime_error(strerror(errno));
}

void ServerConnectionManager::startAcceptConnectionLoop()
{
  is_listening = true;
  while (is_listening)
  {
    Endpoint client_info;
    int socket_descriptor {
      accept(
        descriptor,
        (struct sockaddr*)&client_info.socket_address_information,
        &client_info.socket_address_information_length)
    };

    if (socket_descriptor < 0)
      continue;

    addConnection(socket_descriptor, client_info);

    std::cout << "Client connected successfully." << std::endl;
    std::cout.flush();
  }
}

void ServerConnectionManager::stopAcceptConnectionLoop()
{
  std::lock_guard<std::mutex> lock(mutex);
  is_listening = false;
  shutdown(descriptor, SHUT_RDWR);
  close(descriptor);
}
