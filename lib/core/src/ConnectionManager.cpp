#include <cstdint>
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
#include <unordered_set>
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
  m_is_listen_running{false},
  m_epfd{-1},
  m_command_fd{-1}
{
  m_epfd.store(epoll_create1(0));
  if (m_epfd.load() == -1)
    throw std::runtime_error("couldn't create epoll instance.");
  m_command_fd = eventfd(0,EFD_NONBLOCK | EFD_CLOEXEC);
  if (m_command_fd == -1)
    throw std::runtime_error("eventfd (m_command_fd) error.");

  epoll_event command_event {};
  command_event.events = EPOLLIN;
  command_event.data.fd = m_command_fd;
  if (epoll_ctl(m_epfd, EPOLL_CTL_ADD, m_command_fd, &command_event) == -1)
  {
    close(m_command_fd);
    throw std::runtime_error("eventfd (m_command_fd): couldn't add command fd to epoll.");
  }
}

ConnectionManager::~ConnectionManager()
{
  m_connections.clear();
  for (const int& listen_fd: m_listen_fds) 
  {
    std::cout << listen_fd << std::endl;
    epoll_ctl(
        m_epfd,
        EPOLL_CTL_DEL,
        listen_fd,
        nullptr
    );
    close(listen_fd);
  }
  close(m_command_fd.load());
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

void ConnectionManager::processCommand(AddAllListenerSocketsEventCommand& event_command)
{
  if (event_command.m_listen_fds.empty())
    return;
  std::swap(m_listen_fds, event_command.m_listen_fds);
  for (const int& listen_fd: m_listen_fds)
  {
    std::cout << listen_fd << std::endl;
    epoll_event command_event {};
    command_event.events = EPOLLIN;
    command_event.data.fd = listen_fd;
    epoll_ctl(m_epfd, EPOLL_CTL_ADD, listen_fd, &command_event);
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
  {
    event_command.result.set_value(-1);
    return;
  }

  Connection& conn{*it->second};

  m_message_handler.queueMessage(
    conn,
    event_command.m_message_type,
    event_command.m_payload.data(),
    event_command.m_payload.size()
  );

  updateEpollEvents(conn);

  event_command.result.set_value(0);
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

void ConnectionManager::updateEpollEvents(Connection& conection)
{
  epoll_event ev{};
  ev.events = EPOLLIN;

  if (conection.send_offset < conection.send_buffer.size())
    ev.events |= EPOLLOUT;

  ev.data.fd = conection.socket_descriptor;

  if (epoll_ctl(
    m_epfd,
    EPOLL_CTL_MOD,
    conection.socket_descriptor,
    &ev) == -1)
  {
    std::cerr << "epoll_ctl MOD" << std::endl;
  }
}

void ConnectionManager::initListeners(std::uint16_t port)
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

  std::unordered_set<int> listen_fds;
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

    listen_fds.insert(socket_descriptor);
  }
  freeaddrinfo(server_addresses);

  if (listen_fds.empty())
    throw std::runtime_error("An error has occured for preparing the network addresses");

  {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_event_commands.push(AddAllListenerSocketsEventCommand{listen_fds});
  }
  std::uint64_t value{1};
  if (write(m_command_fd, &value, sizeof(value)) == -1)
  {
    if (errno != EAGAIN && errno != EWOULDBLOCK)
      std::cerr << "write command fd" << std::endl;
  }
}

void ConnectionManager::startListening()
{
  m_is_listen_running.store(true);
}

void ConnectionManager::stopListening()
{
  m_is_listen_running.store(false);
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
      std::cerr << "epoll_wait" << std::endl;
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
            std::cerr << "read command fd" << std::endl;
        }
        processCommands();
        continue;
      }

      {
        auto it {m_listen_fds.find(fd)};
        if (it != m_listen_fds.end())
        {
          const int& fd = *it;
          Endpoint client_info {};
          int client {
            accept(
              fd,
              (struct sockaddr*)&client_info.socket_address_information,
              &client_info.socket_address_information_length)
          };
          if (client == -1)
            continue;
          // Do not accept connection if the m_is_listen_running flag is set to false
          if (!m_is_listen_running.load())
          {
            const char* msg = "SERVER_NOT_ACCEPTING\n";
            ::send(client, msg, std::strlen(msg), 0);
            std::cout << msg;
            shutdown(client, SHUT_RDWR);
            close(client);
            continue;
          }
          {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_event_commands.push(
              AddConnectionEventCommand{
                client,
                client_info
              }
            );
          }
          uint64_t value{1};
          if (write(m_command_fd, &value, sizeof(value)) == -1)
          {
            if (errno != EAGAIN && errno != EWOULDBLOCK)
              std::cerr << "write command fd" << std::endl;
          }
          std::cout << "Client connected successfully." << std::endl;
        }
      }

      auto it = m_connections.find(fd);
      if (it == m_connections.end())
        continue;

      Connection& conn {*it->second};

      if (event.events & EPOLLIN)
      {
        constexpr std::uint16_t buffer_size {4096};
        char receivedBuffer[buffer_size];
        long recv_status {recv(fd, receivedBuffer, sizeof(receivedBuffer)-1, 0)};
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

          long send_status {
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
      std::cerr << "write command fd" << std::endl;
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
      std::cerr << "write command fd" << std::endl;
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
      std::cerr << "write command fd" << std::endl;
  }
}

int ConnectionManager::connect(const std::string &hostname, int port)
{
  // Retrieve the server's network addresses.
  struct addrinfo* server_addresses = nullptr;

  struct addrinfo hints {};
  hints.ai_family = AF_UNSPEC;          // IPv4 or IPv6
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_protocol = IPPROTO_TCP;

  int getaddrinfo_status = getaddrinfo(
      hostname.c_str(),
      std::to_string(port).c_str(),
      &hints,
      &server_addresses);

  if (getaddrinfo_status != 0)
  {
      std::cerr << gai_strerror(getaddrinfo_status) << std::endl;
      return -1;
  }

  int server_socket_descriptor = -1;
  Endpoint endpoint {};

  for (addrinfo* p = server_addresses; p != nullptr; p = p->ai_next)
  {
      int sock = socket(
          p->ai_family,
          p->ai_socktype,
          p->ai_protocol);

      if (sock == -1)
          continue;

      if (::connect(sock, p->ai_addr, p->ai_addrlen) == 0)
      {
          server_socket_descriptor = sock;

          endpoint.socket_address_information_length = p->ai_addrlen;

          std::memcpy(
              &endpoint.socket_address_information,
              p->ai_addr,
              p->ai_addrlen);

          break;
      }

      close(sock);
  }

  freeaddrinfo(server_addresses);

  if (server_socket_descriptor == -1)
      return -2;

  {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_event_commands.push(
      AddConnectionEventCommand{
        server_socket_descriptor,
        endpoint
      }
    );
  }

  uint64_t value{1};
  if (write(m_command_fd, &value, sizeof(value)) == -1)
  {
    if (errno != EAGAIN && errno != EWOULDBLOCK)
      std::cerr << "write command fd" << std::endl;
  }

  return server_socket_descriptor;
}

int ConnectionManager::disconnect(int socket_descriptor)
{
  removeConnection(socket_descriptor);
  return 0;
}

int ConnectionManager::send(
  int socket_descriptor,
  const MessageType& message_type,
  const char* data,
  size_t length
)
{
  std::vector<char> payload(length);
  std::memcpy(payload.data(), data, length);

  std::promise<int> result;
  std::future<int> future {result.get_future()};

  {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_event_commands.push(
      SendMessageEventCommand{
        socket_descriptor,
        message_type,
        std::move(payload),
        std::move(result)
      }
    );
  }

  uint64_t value{1};
  if (write(m_command_fd, &value, sizeof(value)) == -1)
  {
    if (errno != EAGAIN && errno != EWOULDBLOCK)
      std::cerr << "write command fd" << std::endl;
  }

  return future.get();
}

