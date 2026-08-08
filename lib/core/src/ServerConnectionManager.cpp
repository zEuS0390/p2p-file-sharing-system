#include <cerrno>
#include <iostream>
#include <mutex>
#include <netdb.h>
#include <netinet/in.h>
#include <stdexcept>
#include <cstring>
#include <sys/poll.h>
#include <sys/socket.h>
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
  // Create an address information for the server socket
  struct addrinfo* server_addresses {nullptr};

  struct addrinfo hints;
  std::memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_INET6;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_protocol = IPPROTO_TCP;
  hints.ai_flags = AI_PASSIVE;

  std::string port_str {std::to_string(port)};
  int getaddrinfo_status {getaddrinfo(nullptr, port_str.c_str(), &hints, &server_addresses)};

  if (getaddrinfo_status != 0)
    throw std::runtime_error(strerror(errno));

  for (struct addrinfo* p {server_addresses}; p != nullptr; p = p->ai_next)
  {
    int socket_descriptor {
      socket(
        p->ai_family,
        p->ai_socktype,
        p->ai_protocol
      )
    };

    if (socket_descriptor == -1)
      continue;

    // Enable to reuse the same address without the system restruction (ONLY FOR DEV and DEBUGGING) 
    int opt = 1;
    setsockopt(socket_descriptor, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    int v6only = 0;
    setsockopt(socket_descriptor, IPPROTO_IPV6, IPV6_V6ONLY, &v6only, sizeof(v6only));

    // Bind the address information on the server socket
    int bind_result {
      bind(
        socket_descriptor,
        p->ai_addr,
        p->ai_addrlen
      )
    };

    if (bind_result != 0)
    {
      std::cerr << strerror(errno) << std::endl;
      shutdown(socket_descriptor, SHUT_RDWR);
      close(socket_descriptor);
      continue;
    }

    int listen_status {listen(socket_descriptor, SOMAXCONN)};

    if (listen_status != 0)
    {
      std::cerr << strerror(errno) << std::endl;
      shutdown(socket_descriptor, SHUT_RDWR);
      close(socket_descriptor);
      continue;
    }

    listening_socket_descriptors.push_back(socket_descriptor);
  }

  freeaddrinfo(server_addresses);

  if (listening_socket_descriptors.empty())
    throw std::runtime_error("An error has occured for preparing the network addresses");

}

void ServerConnectionManager::startAcceptConnectionLoop()
{
  is_listening = true;

  std::vector<pollfd> poll_fds;

  for (int socket_descriptor: listening_socket_descriptors)
  {
    pollfd pfd {};
    pfd.fd = socket_descriptor;
    pfd.events = POLLIN;
    poll_fds.push_back(pfd);
  }

  while (is_listening)
  {
    int ready {poll(poll_fds.data(), poll_fds.size(), -1)};

    if (ready <= 0)
      continue;

    for (const pollfd& pfd: poll_fds)
    {
      Endpoint client_info {};
      int client {
        accept(
          pfd.fd,
          (struct sockaddr*)&client_info.socket_address_information,
          &client_info.socket_address_information_length)
      };

      if (client == -1)
        continue;

      addConnection(client, client_info);

      std::cout << "Client connected successfully." << std::endl;
    }
  }
}

void ServerConnectionManager::stopAcceptConnectionLoop()
{
  std::lock_guard<std::mutex> lock(mutex);
  is_listening = false;
  for (int socket_descriptor: listening_socket_descriptors)
  {
    shutdown(socket_descriptor, SHUT_RDWR);
    close(socket_descriptor);
  }
}
