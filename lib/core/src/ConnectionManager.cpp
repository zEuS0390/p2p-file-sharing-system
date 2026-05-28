#include <sys/poll.h>
#include <sys/socket.h>
#include <iostream>
#include <unistd.h>
#include <cstring>
#include <netdb.h>
#include <mutex>
#include <unordered_map>
#include <memory>

#include "core/network/ConnectionManager.hpp"
#include "core/network/IMessageHandler.hpp"
#include "core/types/Connection.hpp"
#include "core/utils.hpp"

// Constructor
ConnectionManager::ConnectionManager(IMessageHandler& message_handler):
  descriptor{socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)},
  is_event_running{false},
  message_handler{message_handler}
{
}

ConnectionManager::~ConnectionManager()
{
  // Close the sockets of all connected clients
  for (std::pair<const int, std::shared_ptr<Connection>>& connection: connections)
  {
    std::lock_guard<std::mutex> lock(connection.second->mutex);
    shutdown(connection.second->endpoint.socket_descriptor, SHUT_RDWR);
    close(connection.second->endpoint.socket_descriptor);
  }
  {
    std::lock_guard<std::mutex> lock(mutex);
    connections.clear();
  }
}

void ConnectionManager::parseIncomingMessage(std::shared_ptr<Connection> connection)
{
  while (true)
  {
    // Read the header
    if (connection->reading_header)
    {
      if (connection->recv_buffer.size() <
          sizeof(MessageHeader))
        return;

      // Copy raw bytes from receive buffer into MessageHeader struct
      memcpy(
        &connection->current_header,
        connection->recv_buffer.data(),
        sizeof(MessageHeader)
      );

      connection->recv_buffer.erase(
        connection->recv_buffer.begin(),
        connection->recv_buffer.begin() +
        sizeof(MessageHeader)
      );

      connection->reading_header = false;
    }

    // Read the payload
    if (!connection->reading_header)
    {
      if (connection->recv_buffer.size() <
          connection->current_header.payload_size)
        return;

      {
        std::lock_guard<std::mutex> lock(connection->mutex);
        message_handler.dispatchMessage(
          connection,
          connection->current_header,
          connection->recv_buffer.data()
        );
      }

      connection->recv_buffer.erase(
          connection->recv_buffer.begin(),
          connection->recv_buffer.begin() +
          connection->current_header.payload_size
      );

      connection->reading_header = true;
    }
  }
}

void ConnectionManager::runEventLoop()
{
  is_event_running = true;
  while (is_event_running)
  {
    std::vector<pollfd> connection_pollfds_snapshot;
    {
      std::lock_guard<std::mutex> lock(mutex);
      connection_pollfds_snapshot.reserve(connection_pollfds.size());
      for (pollfd& connection_pollfd: connection_pollfds)
        connection_pollfds_snapshot.push_back(connection_pollfd);
    }

    int ready = poll(connection_pollfds_snapshot.data(), connection_pollfds_snapshot.size(), 5000);

    if (ready > 0)
    {
      for (auto connection_pollfd: connection_pollfds_snapshot)
      {
        std::shared_ptr<Connection> connection;
        std::unordered_map<int, std::shared_ptr<Connection>>::iterator it;
        int connection_socket_descriptor;
        {
          std::lock_guard<std::mutex> lock(mutex);
          it = connections.find(connection_pollfd.fd);
          if (it == connections.end())
            continue;
        }
        {
          std::lock_guard<std::mutex> lock(it->second->mutex);
          connection = it->second;
          connection_socket_descriptor = connection_pollfd.fd;
        }

        if (connection_pollfd.revents & (POLLHUP | POLLERR | POLLNVAL))
        {
          std::cout << "socket error or hangup." << std::endl;
          std::cout.flush();
          {
            std::lock(mutex, connection->mutex);
            std::lock_guard<std::mutex> lock1(mutex, std::adopt_lock);
            std::lock_guard<std::mutex> lock(connection->mutex, std::adopt_lock);
            shutdown(connection_socket_descriptor, SHUT_RDWR);
            close(connection_socket_descriptor);
            connections.erase(connection_pollfd.fd);
            removePollFD(connection_pollfds, connection_socket_descriptor);
          }
        }
        else if (connection_pollfd.revents & POLLIN)
        {

          char buffer[4096] {};
          ssize_t recv_status = recv(connection_pollfd.fd, buffer, sizeof(buffer)-1, 0);

          if (recv_status > 0)
          {
            {
              std::lock_guard<std::mutex> lock(connection->mutex);
              connection->recv_buffer.insert(
                  connection->recv_buffer.end(),
                  buffer,
                  buffer + recv_status
              );
            }
            parseIncomingMessage(connection);
          }
          else if (recv_status == 0)
          {
            std::cout << "disconnected cleanly." << std::endl;
            std::cout.flush();
            std::lock_guard<std::mutex> lock1(mutex);
            shutdown(connection_socket_descriptor, SHUT_RDWR);
            close(connection_socket_descriptor);
            connections.erase(connection_pollfd.fd);
            removePollFD(connection_pollfds, connection_socket_descriptor);
          }
        }
      }
    }
  }
}

void ConnectionManager::stopEventLoop()
{
  is_event_running = false;
}

void ConnectionManager::addConnection(
  int socket_desrciptor,
  std::shared_ptr<Connection> connection,
  pollfd connection_pollfd
)
{
  std::lock(mutex, connection->mutex);
  std::lock_guard<std::mutex> lock1(mutex, std::adopt_lock);
  std::lock_guard<std::mutex> lock2(connection->mutex, std::adopt_lock);
  connection_pollfds.push_back(connection_pollfd);
  connections[socket_desrciptor] = connection;
}

void ConnectionManager::removeConnection(
  int socket_descriptor
)
{
  std::shared_ptr<Connection> connection;
  {
    std::lock_guard<std::mutex> lock(mutex);
    std::unordered_map<int, std::shared_ptr<Connection>>::iterator it = connections.find(socket_descriptor);
    if (it == connections.end())
      return;
    connection = it->second;
  }
  {
    std::lock_guard<std::mutex> lock(connection->mutex);
    shutdown(connection->endpoint.socket_descriptor, SHUT_RDWR);
    close(connection->endpoint.socket_descriptor);
  }
  {
    std::lock_guard<std::mutex> lock1(mutex);
    connections.erase(socket_descriptor);
  }
}

