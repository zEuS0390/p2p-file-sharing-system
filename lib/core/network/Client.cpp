#include <netinet/in.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <vector>

#include "core/network/Client.hpp"
#include "core/types/Endpoint.hpp"

// Constructor
Client::Client()
{
}

// Destructor
Client::~Client()
{
  for (Endpoint& server: servers)
  {
    shutdown(server.socket_descriptor, SHUT_RDWR);
    close(server.socket_descriptor);
  }
  servers.clear();
}

// Connect to the server with the given hostname and port
int Client::connectToServer(const std::string& hostname, int port)
{
  struct addrinfo* server_address {};
  struct addrinfo hints {};

  hints.ai_family = AF_INET;
  hints.ai_protocol = IPPROTO_TCP;
  hints.ai_socktype = SOCK_STREAM;

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

  int server_socket_descriptor {
    socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)
  };

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

  struct sockaddr_in* socket_address = (struct sockaddr_in*)server_address->ai_addr;
  unsigned int socket_address_length = server_address->ai_addrlen;
  Endpoint endpoint {server_socket_descriptor, *socket_address, socket_address_length};
  servers.push_back(endpoint);

  freeaddrinfo(server_address);

  return server_socket_descriptor;
}

// Disconnect to the server
int Client::disconnectToServer(int socket_descriptor)
{
  for (std::vector<Endpoint>::iterator it = servers.begin();
       it != servers.end();
      )
  {
    if (it->socket_descriptor == socket_descriptor)
    {
      shutdown(it->socket_descriptor, SHUT_RDWR);
      close(it->socket_descriptor);
      it = servers.erase(it);
    }
    else
    {
      ++it;
    }
  }
  return 0;
}
