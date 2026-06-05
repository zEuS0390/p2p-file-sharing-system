#include <iostream>
#include <stdexcept>
#include <cstring>
#include <unistd.h>

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

  // Create an address information for the server socket
  struct sockaddr_in server_address {};
  server_address.sin_family = AF_INET;
  server_address.sin_port = htons(port);
  server_address.sin_addr.s_addr = INADDR_ANY;

  // Bind the address information on the server socket
  int bind_result {
    bind(
      descriptor,
      (const struct sockaddr*)&server_address,
      sizeof(server_address)
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
  is_listening = false;
  shutdown(descriptor, SHUT_RDWR);
  close(descriptor);
}
