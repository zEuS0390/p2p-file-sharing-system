#include "netdb.h"

#include "p2p_fss/network/Client.hpp"

Client::Client()
{
}

Client::~Client()
{
}

void Client::initListeners(std::uint16_t port)
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
    std::lock_guard lock(m_mutex);
    m_commands.push(Command::AddAllListenerSockets{listen_fds});
  }
  std::uint64_t value{1};
  m_io_multiplexer.writeFD(m_command_fd, &value, sizeof(value));
}
