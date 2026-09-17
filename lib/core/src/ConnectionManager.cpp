#include <unordered_map>
#include <sys/eventfd.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <sys/poll.h>
#include <algorithm>
#include <stdexcept>
#include <iostream>
#include <unistd.h>
#include <cstring>
#include <netdb.h>
#include <utility>
#include <cstdio>
#include <memory>
#include <cerrno>
#include <mutex>
#include <array>
#include <queue>

#include "core/network/ConnectionManager.hpp"
#include "core/network/IMessageHandler.hpp"
#include "core/types/Connection.hpp"
#include "core/types/MessageHeaders.hpp"
#include "core/types/EventCommand.hpp"

// Constructor
ConnectionManager::ConnectionManager(IMessageHandler& message_handler):
  m_message_handler{message_handler},
  m_is_event_running{false},
  m_epfd{-1},
  m_command_fd{-1}
{
  m_epfd.store(epoll_create1(0));
  if (m_epfd.load() == -1)
    throw std::runtime_error("couldn't create epoll instance.");
  m_command_fd = eventfd(0,EFD_NONBLOCK | EFD_CLOEXEC);
  if (m_command_fd == -1)
    throw std::runtime_error("eventfd error.");

  epoll_event command_event {};
  command_event.events = EPOLLIN;
  command_event.data.fd = m_command_fd;

  if (epoll_ctl(m_epfd, EPOLL_CTL_ADD, m_command_fd, &command_event) == -1)
  {
    close(m_command_fd);
    throw std::runtime_error("couldn't add command fd to epoll.");
  }
}

ConnectionManager::~ConnectionManager()
{
  m_connections.clear();
  close(m_epfd.load());
}

void ConnectionManager::parseIncomingMessage(
  Connection& connection
)
{
  while (true)
  {
    // Read the header
    if (connection.reading_header)
    {
      if (connection.recv_buffer.size() - connection.recv_offset < sizeof(MessageHeader))
        return;

      std::memcpy(
        &connection.current_header,
        connection.recv_buffer.data() + connection.recv_offset,
        sizeof(MessageHeader)
      );

      connection.recv_offset += sizeof(MessageHeader);
      connection.reading_header = false;
    }

    // Read the payload
    if (!connection.reading_header)
    {
      if (connection.recv_buffer.size() - connection.recv_offset < connection.current_header.payload_size)
        return;

      m_message_handler.dispatchMessage(
        connection,
        connection.current_header,
        connection.recv_buffer.data() + connection.recv_offset
      );

      updateEpollEvents(connection);

      connection.recv_offset += connection.current_header.payload_size;
      connection.reading_header = true;
    }

    if (connection.recv_offset == connection.recv_buffer.size())
    {
      connection.recv_buffer.clear();
      connection.recv_offset = 0;
    }
  }
}

void ConnectionManager::processCommand(AddConnectionEventCommand& event_command)
{
  std::unique_ptr<Connection> connection{
      std::make_unique<Connection>()
  };

  connection->endpoint = event_command.endpoint;
  connection->socket_descriptor = event_command.m_socket_descriptor;

  epoll_event connection_epoll_event{};
  connection_epoll_event.events = EPOLLIN;
  connection_epoll_event.data.fd = event_command.m_socket_descriptor;

  if (epoll_ctl(
        m_epfd,
        EPOLL_CTL_ADD,
        event_command.m_socket_descriptor,
        &connection_epoll_event
      ) == -1)
  {
      std::cout
          << "couldn't add file descriptor to epoll instance."
          << std::endl;

      shutdown(event_command.m_socket_descriptor, SHUT_RDWR);
      close(event_command.m_socket_descriptor);
      return;
  }

  m_connections.emplace(
      event_command.m_socket_descriptor,
      std::move(connection)
  );
}

void ConnectionManager::processCommand(RemoveConnectionEventCommand& event_command)
{
  auto it = m_connections.find(event_command.m_socket_descriptor);

  if (it == m_connections.end())
    return;

  Connection& conn{*it->second};

  epoll_ctl(
      m_epfd,
      EPOLL_CTL_DEL,
      conn.socket_descriptor,
      nullptr
  );

  shutdown(conn.socket_descriptor, SHUT_RDWR);
  close(conn.socket_descriptor);

  m_connections.erase(it);
}

void ConnectionManager::processCommand(SendMessageEventCommand& event_command)
{
  auto it = m_connections.find(event_command.m_socket_descriptor);

  if (it == m_connections.end())
    return;

  Connection& conn{*it->second};

  m_message_handler.queueMessage(
    conn,
    event_command.m_message_type,
    event_command.m_payload.data(),
    event_command.m_payload.size()
  );

  updateEpollEvents(conn);
}

void ConnectionManager::processCommands() 
{
  std::queue<EventCommand> event_commands;

  {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::swap(event_commands, m_event_commands);
  }

  while (!event_commands.empty())
  {
    EventCommand event_command {std::move(event_commands.front())};
    event_commands.pop();

    std::visit(
      [&](auto& command)
      {
        processCommand(command);
      },
      event_command
    );
  }
}

void ConnectionManager::updateEpollEvents(Connection& conn)
{
  epoll_event ev{};
  ev.events = EPOLLIN;

  if (conn.send_offset < conn.send_buffer.size())
    ev.events |= EPOLLOUT;

  ev.data.fd = conn.socket_descriptor;

  if (epoll_ctl(
    m_epfd,
    EPOLL_CTL_MOD,
    conn.socket_descriptor,
    &ev) == -1)
  {
    perror("epoll_ctl MOD");
  }
}

void ConnectionManager::runEventLoop()
{
  m_is_event_running.store(true);
  while (m_is_event_running.load())
  {
    int ready {epoll_wait(m_epfd, m_epoll_events.data(), MAX_EVENTS, -1)};

    if (ready == -1)
    {
      if (errno == EINTR)
        continue;
      perror("epoll_wait");
      break;
    }

    for (int i {0}; i < ready; ++i)
    {
      epoll_event& event {m_epoll_events.at(i)};
      int fd {event.data.fd};

      if (fd == m_command_fd)
      {
        uint64_t value;
        if (read(m_command_fd, &value, sizeof(value)) == -1)
        {
          if (errno != EAGAIN && errno != EWOULDBLOCK)
            perror("read command fd");
        }
        processCommands();
        continue;
      }

      std::unordered_map<int, std::unique_ptr<Connection>>::iterator it;
      {
        it = m_connections.find(fd);
        if (it == m_connections.end())
          continue;
      }

      Connection& conn {*it->second};

      if (event.events & EPOLLIN)
      {
        char receivedBuffer[4096];
        ssize_t recv_status {recv(fd, receivedBuffer, sizeof(receivedBuffer)-1, 0)};
        if (recv_status > 0)
        {
          conn.recv_buffer.insert(
            conn.recv_buffer.end(),
            receivedBuffer,
            receivedBuffer + recv_status
          );

          parseIncomingMessage(conn);
        }
        else if (recv_status == 0)
        {
          std::cout << "disconnected cleanly." << std::endl;

          epoll_ctl(
            m_epfd,
            EPOLL_CTL_DEL,
            conn.socket_descriptor,
            nullptr
          );

          shutdown(conn.socket_descriptor, SHUT_RDWR);
          close(conn.socket_descriptor);

          m_connections.erase(it);

          continue;
        }
        else
        {
          if (errno != EAGAIN && errno != EWOULDBLOCK && errno != EINTR)
          {
            removeConnection(fd);
            continue;
          }
        }
      }
      if (event.events & EPOLLOUT)
      {
        while (conn.send_offset < conn.send_buffer.size())
        {
          size_t remaining {conn.send_buffer.size() - conn.send_offset};

          ssize_t send_status {
            ::send(
              fd,
              conn.send_buffer.data() + conn.send_offset,
              remaining,
              0
            )
          };

          if (send_status > 0)
          {
            conn.send_offset += send_status;
            continue;
          }

          if (send_status == -1)
          {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
              break;

            if (errno == EINTR)
              continue;

            std::cout << "send error." << std::endl;
            removeConnection(fd);
            break;
          }
        }

        if (conn.send_offset == conn.send_buffer.size())
        {
          conn.send_buffer.clear();
          conn.send_offset = 0;
          updateEpollEvents(conn);
        }
      }
      if (event.events & EPOLLHUP)
      {
        std::cout << "socket hangup." << std::endl;
        removeConnection(fd);
        continue;
      }
      if (event.events & EPOLLERR)
      {
        std::cout << "socket error." << std::endl;
        removeConnection(fd);
        continue;
      }
    }
  }
}

void ConnectionManager::stopEventLoop()
{
  m_is_event_running.store(false);

  uint64_t value{1};

  if (write(m_command_fd, &value, sizeof(value)) == -1)
  {
    if (errno != EAGAIN && errno != EWOULDBLOCK)
      perror("write command fd");
  }
}

void ConnectionManager::addConnection(
  int socket_descriptor,
  const Endpoint& endpoint
)
{
  {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_event_commands.push(
      AddConnectionEventCommand{
        socket_descriptor,
        endpoint
      }
    );
  }

  uint64_t value{1};
  if (write(m_command_fd, &value, sizeof(value)) == -1)
  {
    if (errno != EAGAIN && errno != EWOULDBLOCK)
      perror("write command fd");
  }
  return;
}

void ConnectionManager::removeConnection(int socket_descriptor)
{
  {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_event_commands.push(
      RemoveConnectionEventCommand{
        socket_descriptor
      }
    );
  }
  uint64_t value{1};
  if (write(m_command_fd, &value, sizeof(value)) == -1)
  {
    if (errno != EAGAIN && errno != EWOULDBLOCK)
      perror("write command fd");
  }
}

ssize_t ConnectionManager::send(
  int socket_descriptor,
  const MessageType& message_type,
  const char* data,
  size_t length
)
{
  std::vector<char> payload(length);

  std::memcpy(payload.data(), data, length);

  {
    std::lock_guard<std::mutex> lock(m_mutex);

    m_event_commands.push(
      SendMessageEventCommand{
        socket_descriptor,
        message_type,
        std::move(payload)
      }
    );
  }

  uint64_t value{1};
  if (write(m_command_fd, &value, sizeof(value)) == -1)
  {
    if (errno != EAGAIN && errno != EWOULDBLOCK)
      perror("write command fd");
  }

  return 0;
}

