#include <netinet/in.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <unordered_map>

#include "core/network/Client.hpp"
#include "core/types/Endpoint.hpp"

// Constructor
Client::Client()
{
}

// Destructor
Client::~Client()
{
  // Close the sockets of all connected clients
  for (std::pair<const int, Endpoint>& server: servers)
  {
    shutdown(server.second.socket_descriptor, SHUT_RDWR);
    close(server.second.socket_descriptor);
  }
  servers.clear();
}

// Connect to the server with the given hostname and port
int Client::connectToServer(const std::string& hostname, int port)
{
  // Declare a variable that points to an address information structure
  // that will be populated when the getaddrinfo function is invoked.
  struct addrinfo* server_address {};

  // Declare an address information structure that will be used
  // to filter and define the type of network address that the
  // getadrinfo function returns.
  struct addrinfo hints {};

  hints.ai_family = AF_INET;
  hints.ai_protocol = IPPROTO_TCP;
  hints.ai_socktype = SOCK_STREAM;

  // Translate a human-readable hostname and port into a
  // a format that a computer can use to established a
  // network connection.
  int get_addrinfo_status {
    getaddrinfo(hostname.c_str(),
    std::to_string(port).c_str(),
    &hints,
    &server_address)
  };

  if (get_addrinfo_status < 0)
  {
    freeaddrinfo(server_address);
    return -1;
  }

  // Create an endpoint for communication. It acts like
  // opening a file so the operating system can prepare
  // a channel for sending and receving data accross a
  // network.
  int server_socket_descriptor {
    socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)
  };

  // Establish a connection to be able to communicate
  int connect_status {
    connect(
      server_socket_descriptor,
      server_address->ai_addr,
      server_address->ai_addrlen)
  };

  if (connect_status < 0)
  {
    freeaddrinfo(server_address);
    return -2;
  }

  // Create a socket address information of the server
  struct sockaddr_in* socket_address = (struct sockaddr_in*)server_address->ai_addr;
  unsigned int socket_address_length = server_address->ai_addrlen;
  Endpoint endpoint {server_socket_descriptor, *socket_address, socket_address_length};

  // Add the socket address information in the server list
  // servers.push_back(endpoint);
  servers[server_socket_descriptor] = endpoint;

  freeaddrinfo(server_address);

  return server_socket_descriptor;
}

// Disconnect to the server
int Client::disconnectToServer(int socket_descriptor)
{
  std::unordered_map<int, Endpoint>::iterator it {servers.find(socket_descriptor)};
  if (it != servers.end())
  {
    shutdown(it->second.socket_descriptor, SHUT_RDWR);
    close(it->second.socket_descriptor);
    it = servers.erase(it);
  }
  return 0;
}

ssize_t Client::sendAll(int socket_descriptor, const char* data, size_t length)
{
  std::unordered_map<int, Endpoint>::iterator it {servers.find(socket_descriptor)};
  if (it == servers.end())
  {
    return -1;
  }

  size_t total {0};

  while (total < length)
  {
    ssize_t sent {send(socket_descriptor, data + total, length - total, 0)};
    
    if (sent <= 0)
      return -1;

    total += sent;
  }

  return 0;
}
