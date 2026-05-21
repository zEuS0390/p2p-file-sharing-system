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

#include "core/network/Server.hpp"
#include "core/types/ClientConnection.hpp"
#include "core/types/Endpoint.hpp"
#include "core/types/MessageHeaders.hpp"
#include "core/utils.hpp"

// Constructor
Server::Server():
  is_listening{false},
  is_monitoring{false},
  listen_limit(SOMAXCONN)
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
    throw std::runtime_error(strerror(errno));

  int listen_status {listen(descriptor, listen_limit)};

  if (listen_status < 0)
    throw std::runtime_error(strerror(errno));
}

// Destructor
Server::~Server()
{
  {
    std::lock_guard<std::mutex> lock(mutex);
    for (std::pair<const int, ClientConnection>& client: clients)
    {
      shutdown(client.second.endpoint.socket_descriptor, SHUT_RDWR);
      close(client.second.endpoint.socket_descriptor);
    }
    clients.clear();
  }
  shutdown(descriptor, SHUT_RDWR);
  close(descriptor);
}

void Server::dispatchMessage(
  ClientConnection* client_connection,
  MessageHeader& message_header,
  const char* data
)
{
  switch(message_header.type)
  {
    case MessageType::MESSAGE:
      std::cout << data;
      std::cout.flush();
      break;
    default:
      break;
  }
}

// Parse received message for the connected client
void Server::parseMessage(ClientConnection* client_connection)
{
  while (true)
  {
    // Read the header
    if (client_connection->reading_header)
    {
      if (client_connection->recv_buffer.size() <
          sizeof(MessageHeader))
        return;

      // Copy raw bytes from receive buffer into MessageHeader struct
      memcpy(
        &client_connection->current_header,
        client_connection->recv_buffer.data(),
        sizeof(MessageHeader)
      );

      client_connection->recv_buffer.erase(
        client_connection->recv_buffer.begin(),
        client_connection->recv_buffer.begin() +
        sizeof(MessageHeader)
      );

      client_connection->reading_header = false;
    }

    // Read the payload
    if (!client_connection->reading_header)
    {
      if (client_connection->recv_buffer.size() <
          client_connection->current_header.payload_size)
        return;

      dispatchMessage(
        client_connection,
        client_connection->current_header,
        client_connection->recv_buffer.data()
      );

      client_connection->recv_buffer.erase(
          client_connection->recv_buffer.begin(),
          client_connection->recv_buffer.begin() +
          client_connection->current_header.payload_size
      );

      client_connection->reading_header = true;
    }
  }
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
      clients[client_socket_descriptor].endpoint = client_info;
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
    std::vector<pollfd> client_pollfds_snapshot;
    client_pollfds_snapshot.reserve(client_pollfds.size());

    {
      std::lock_guard<std::mutex> lock(mutex);
      for (pollfd& client_pollfd: client_pollfds)
        client_pollfds_snapshot.push_back(client_pollfd);
    }

    int ready = poll(client_pollfds_snapshot.data(), client_pollfds_snapshot.size(), 5000);

    if (ready > 0)
    {
      for (size_t i = 0; i < client_pollfds_snapshot.size(); ++i)
      {
        pollfd& client_pollfd {client_pollfds_snapshot.at(i)};

        if (client_pollfd.revents & POLLIN)
        {
          ClientConnection* client_connection {};
          {
            std::lock_guard<std::mutex> lock(mutex);
             client_connection = &clients.at(client_pollfd.fd);
          }

          char buffer[4096] {};
          ssize_t recv_status = recv(client_pollfd.fd, buffer, sizeof(buffer)-1, 0);

          if (recv_status > 0)
          {
            std::lock_guard<std::mutex> lock(mutex);
            client_connection->recv_buffer.insert(
                client_connection->recv_buffer.end(),
                buffer,
                buffer + recv_status
            );
            parseMessage(client_connection);
          }
          else if (recv_status == 0)
          {
            std::cout << "Client disconnected cleanly." << std::endl;
            std::cout.flush();
            shutdown(client_pollfd.fd, SHUT_RDWR);
            close(client_pollfd.fd);
            std::lock_guard<std::mutex> lock(mutex);
            clients.erase(client_pollfd.fd);
            removePollFD(client_pollfds, client_pollfd.fd);
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
            removePollFD(client_pollfds, client_pollfd.fd);
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
