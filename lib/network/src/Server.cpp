#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdexcept>
#include <unistd.h>
#include <iostream>
#include <netdb.h>
#include <cstring>
#include <utility>
#include <vector>
#include <cerrno>
#include <mutex>

#include "network/Server.hpp"
#include "types/Endpoint.hpp"

// Constructor
Server::Server():
  is_listening{false},
  is_monitoring{false}
{
  // Enable to reuse the same address without the system restruction (ONLY FOR DEV and DEBUGGING) 
  int opt = 1;
  setsockopt(descriptor, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

  // Create an address information for the server socket
  struct sockaddr_in server_address;
  server_address.sin_family = AF_INET;
  server_address.sin_port = htons(12345);
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
    throw std::runtime_error(
      "There was an error binding the address and port on the socket."
    );

  listen(descriptor, 10);
}

// Destructor
Server::~Server()
{
  {
    std::lock_guard<std::mutex> lock(mutex);
    for (std::pair<const int, Endpoint>& client: clients)
    {
      shutdown(client.second.socket_descriptor, SHUT_RDWR);
      close(client.second.socket_descriptor);
    }
    clients.clear();
  }
  shutdown(descriptor, SHUT_RDWR);
  close(descriptor);
}

// Listen for incoming client connections
void Server::startListening()
{
  is_listening = true;
  while (is_listening)
  {
    Endpoint client_info;
    int client_socket_descriptor {
      accept(
        descriptor,
        (struct sockaddr*)&client_info.socket_address_information,
        &client_info.socket_address_information_length)
    };

    if (client_socket_descriptor < 0)
      continue;

    {
      std::lock_guard<std::mutex> lock(mutex);
      pollfd client_pollfd;
      client_pollfd.fd = client_socket_descriptor;
      client_pollfd.events = POLLIN;
      client_pollfd.revents = 0;
      client_pollfds.push_back(client_pollfd);
      clients[client_socket_descriptor] = client_info;
    }
    std::cout << "Client connected successfully." << std::endl;
    std::cout.flush();
  }
}

// Stop listening for incoming client conncections
void Server::stopListening()
{
  is_listening = false;
  shutdown(descriptor, SHUT_RDWR);
  close(descriptor);
}

// Get the number of connected clients
int Server::getNumberOfClients()
{
  std::lock_guard<std::mutex> lock(mutex);
  return clients.size();
}

// Monitor the statuses of connected clients
void Server::startMonitoring()
{
  is_monitoring = true;
  while (is_monitoring)
  {
    std::vector<pollfd> snapshot;
    snapshot.reserve(client_pollfds.size());

    {
      std::lock_guard<std::mutex> lock(mutex);
      for (pollfd& client_pollfd: client_pollfds)
        snapshot.push_back(client_pollfd);
    }

    int ready = poll(snapshot.data(), snapshot.size(), 5000);

    if (ready > 0)
    {
      for (size_t i = 0; i < snapshot.size(); ++i)
      {
        pollfd& client_pollfd {snapshot.at(i)};

        if (client_pollfd.revents & POLLIN)
        {
          char buffer[256] {};
          ssize_t recv_status = recv(client_pollfd.fd, buffer, sizeof(buffer)-1, 0);

          if (recv_status > 0)
          {
            std::cout << buffer << std::endl;
          }
          else if (recv_status == 0)
          {
            std::cout << "Client disconnected cleanly." << std::endl;
            std::cout.flush();
            shutdown(client_pollfd.fd, SHUT_RDWR);
            close(client_pollfd.fd);
            {
              std::lock_guard<std::mutex> lock(mutex);
              clients.erase(client_pollfd.fd);
            }
          }
        }

        if (client_pollfd.revents & (POLLHUP | POLLERR))
        {
          std::cout << "Client socket error or hangup." << std::endl;
          std::cout.flush();
          shutdown(client_pollfd.fd, SHUT_RDWR);
          close(client_pollfd.fd);
          {
            std::lock_guard<std::mutex> lock(mutex);
            clients.erase(client_pollfd.fd);
          }
        }
      }
    }
  }
}

// Stop monitoring the statuses of connected clients
void Server::stopMonitoring()
{
  is_monitoring = false;
}
