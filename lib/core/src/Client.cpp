#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netdb.h>
#include <iostream>
#include <cstring>

#include "core/network/Client.hpp"
#include "core/network/IMessageHandler.hpp"
#include "core/types/Endpoint.hpp"
#include "core/types/MessageType.hpp"

// Constructor
Client::Client(IMessageHandler& message_handler) noexcept:
  m_client_connection_manager{message_handler}
{
}

// Destructor
Client::~Client()
{
}

// Connect to the server with the given hostname and port.
int Client::connect(const std::string& hostname, int port)
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

    m_client_connection_manager.addConnection(
        server_socket_descriptor,
        endpoint);

    return server_socket_descriptor;
}

// Disconnect to the server
int Client::disconnect(int socket_descriptor)
{
  m_client_connection_manager.removeConnection(socket_descriptor);
  return 0;
}

int Client::send(int socket_descriptor, const MessageType& message_type, const char* data, size_t length)
{
  return m_client_connection_manager.send(socket_descriptor, message_type, data, length);
}

void Client::start()
{
  m_event_thread = std::thread{&ClientConnectionManager::runEventLoop, std::ref(m_client_connection_manager)};
}

void Client::stop()
{
  m_client_connection_manager.stopEventLoop();
  m_event_thread.join();
}

