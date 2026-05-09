#include <sys/socket.h>
#include <netdb.h>

#include "network/Socket.hpp"

// Constructor
Socket::Socket():
  descriptor{socket(AF_INET,SOCK_STREAM,IPPROTO_TCP)}
{
}

// Get the file descriptor
int Socket::getDescriptor() const
{
  return descriptor;
}
