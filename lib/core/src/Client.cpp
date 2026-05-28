#include <memory>
#include <netinet/in.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <iostream>

#include "core/network/Client.hpp"
#include "core/network/IMessageHandler.hpp"
#include "core/types/Connection.hpp"
#include "core/types/Endpoint.hpp"

// Constructor
Client::Client(IMessageHandler& message_handler):
  client_connection_manager{message_handler}
{
}

// Destructor
Client::~Client()
{
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
  Endpoint endpoint;
  endpoint.socket_descriptor = server_socket_descriptor;
  endpoint.socket_address_information_length = socket_address_length;
  endpoint.socket_address_information = *socket_address;

  // Add the socket address information in the connections
  std::shared_ptr<Connection> connection = std::make_shared<Connection>();
  connection->endpoint = endpoint;

  pollfd client_pollfd;
  client_pollfd.fd = server_socket_descriptor;
  client_pollfd.events = POLLIN;
  client_pollfd.revents = 0;

  client_connection_manager.addConnection(
    server_socket_descriptor,
    connection,
    client_pollfd
  );

  freeaddrinfo(server_address);

  return server_socket_descriptor;
}

// Disconnect to the server
int Client::disconnectToServer(int socket_descriptor)
{
  client_connection_manager.removeConnection(socket_descriptor);
  return 0;
}

ssize_t Client::sendAll(int socket_descriptor, const char* data, size_t length)
{
  size_t total {0};

  while (total < length)
  {
    ssize_t sent {send(socket_descriptor, data + total, length - total, MSG_NOSIGNAL)};
    
    if (sent <= 0)
    {
      std::cerr << "Error sending the message." << std::endl;
      return -1;
    }

    total += sent;
  }

  return total;
}

void Client::start()
{
  event_thread = std::thread{&ClientConnectionManager::runEventLoop, std::ref(client_connection_manager)};
}

void Client::stop()
{
  client_connection_manager.stopEventLoop();
  event_thread.join();
}
