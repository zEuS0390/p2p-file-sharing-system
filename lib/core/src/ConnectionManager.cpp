#include <unordered_map>
#include <sys/socket.h>
#include <sys/poll.h>
#include <iostream>
#include <unistd.h>
#include <cstring>
#include <netdb.h>
#include <memory>
#include <cerrno>
#include <mutex>

#include "core/network/ConnectionManager.hpp"
#include "core/network/IMessageHandler.hpp"
#include "core/types/Connection.hpp"
#include "core/types/MessageHeaders.hpp"

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
    shutdown(connection.second->pollfd.fd, SHUT_RDWR);
    close(connection.second->pollfd.fd);
  }
  connections.clear();
}

void ConnectionManager::parseIncomingMessage(
  std::shared_ptr<Connection> connection
)
{
  while (true)
  {
    // Read the header
    if (connection->reading_header)
    {
      if (connection->recv_buffer.size() - connection->recv_offset < sizeof(MessageHeader))
        return;

      std::memcpy(
        &connection->current_header,
        connection->recv_buffer.data() + connection->recv_offset,
        sizeof(MessageHeader)
      );

      connection->recv_offset += sizeof(MessageHeader);
      connection->reading_header = false;
    }

    // Read the payload
    if (!connection->reading_header)
    {
      if (connection->recv_buffer.size() - connection->recv_offset < connection->current_header.payload_size)
        return;

      message_handler.dispatchMessage(
        connection,
        connection->current_header,
        connection->recv_buffer.data() + connection->recv_offset
      );

      connection->recv_offset += connection->current_header.payload_size;
      connection->reading_header = true;
    }

    if (connection->recv_offset == connection->recv_buffer.size())
    {
      connection->recv_buffer.clear();
      connection->recv_offset = 0;
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
      connection_pollfds_snapshot.reserve(connections.size());
      for (auto& it: connections)
        connection_pollfds_snapshot.push_back(it.second->pollfd);
    }

    int ready = poll(
      connection_pollfds_snapshot.data(),
      connection_pollfds_snapshot.size(),
      10
    );

    if (ready <= 0)
      continue;

    for (auto connection_pollfd: connection_pollfds_snapshot)
    {
      std::shared_ptr<Connection> connection;

      {
        std::lock_guard<std::mutex> lock(mutex);
        auto it {connections.find(connection_pollfd.fd)};
        if (it == connections.end())
          continue;
        // std::lock_guard<std::mutex> lock2(it->second->mutex);
        connection = it->second;
      }

      {
        std::lock_guard<std::mutex> lock(connection->mutex);
        if (connection->send_offset < connection->send_buffer.size())
          connection->pollfd.events |= POLLOUT;
        else
          connection->pollfd.events &= ~POLLOUT;
      }

      if (connection_pollfd.revents & (POLLHUP | POLLERR | POLLNVAL))
      {
        std::cout << "socket error or hangup." << std::endl;
        std::cout.flush();
        removeConnection(connection->pollfd.fd);
        continue;
      }

      if (connection_pollfd.revents & POLLIN)
      {
        char buffer[4096] {};
        ssize_t recv_status = recv(connection_pollfd.fd, buffer, sizeof(buffer)-1, 0);
        if (recv_status > 0)
        {
          std::lock_guard<std::mutex> lock(connection->mutex);
          connection->recv_buffer.insert(
              connection->recv_buffer.end(),
              buffer,
              buffer + recv_status
          );
          parseIncomingMessage(connection);
        }
        else if (recv_status == 0)
        {
          std::cout << "disconnected cleanly." << std::endl;
          std::cout.flush();
          removeConnection(connection->pollfd.fd);
          continue;
        }
        else
        {
          if (errno == EAGAIN || errno == EWOULDBLOCK)
            continue;
          if (errno == EINTR)
            continue;
          removeConnection(connection->pollfd.fd);
          continue;
        }
      }

      if (connection_pollfd.revents & POLLOUT)
      {
        std::lock_guard<std::mutex> lock(connection->mutex);
        size_t remaining {connection->send_buffer.size() - connection->send_offset};
        if (remaining > 0)
        {
          ssize_t bytes_sent = ::send(
            connection->pollfd.fd,
            connection->send_buffer.data() + connection->send_offset,
            remaining,
            0
          );
          if (bytes_sent > 0)
            connection->send_offset += bytes_sent;
          else if (bytes_sent == 0)
            continue;
        }
        if (connection->send_offset == connection->send_buffer.size())
        {
          connection->send_buffer.clear();
          connection->send_offset = 0;
          connection->pollfd.events &= ~POLLOUT;
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
  const Endpoint& endpoint
)
{
  std::lock_guard<std::mutex> lock1(mutex);

  pollfd conn_pollfd;
  conn_pollfd.fd = socket_desrciptor;
  conn_pollfd.events = POLLIN;
  conn_pollfd.revents = 0;

  // Add the socket address information in the connections
  std::shared_ptr<Connection> connection = std::make_shared<Connection>();
  connection->endpoint = endpoint;
  connection->pollfd = conn_pollfd;
  connections[socket_desrciptor] = connection;
}

void ConnectionManager::removeConnection(int socket_descriptor)
{
  std::lock_guard<std::mutex> lock1(mutex);
  std::shared_ptr<Connection> connection;
  std::unordered_map<int, std::shared_ptr<Connection>>::iterator it {
    connections.find(socket_descriptor)
  };
  if (it == connections.end())
    return;
  connection = it->second;
  std::lock_guard<std::mutex> lock2(connection->mutex);
  shutdown(connection->pollfd.fd, SHUT_RDWR);
  close(connection->pollfd.fd);
  connections.erase(connection->pollfd.fd);
}

ssize_t ConnectionManager::send(
  int socket_descriptor,
  const MessageType& message_type,
  const char* data,
  size_t length
)
{
  std::shared_ptr<Connection> connection;
  {
    std::lock_guard<std::mutex> lock(mutex);
    auto it {connections.find(socket_descriptor)};
    if (it == connections.end())
      return -1;
    connection = it->second;
  }
  {
    std::lock(mutex, connection->mutex);
    std::lock_guard<std::mutex> lock1(mutex, std::adopt_lock);
    std::lock_guard<std::mutex> lock2(connection->mutex, std::adopt_lock);
    message_handler.queueMessage(connection, message_type, data, length);
  }
  return 0;
}

