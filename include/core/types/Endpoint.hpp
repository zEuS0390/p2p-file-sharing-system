#ifndef CORE_TYPES_ENDPOINT_HPP
#define CORE_TYPES_ENDPOINT_HPP

#include <netinet/in.h>
#include <poll.h>

/*
 * A structure containing the address information of the connection.
 */
struct Endpoint
{
  struct sockaddr_storage socket_address_information {};
  unsigned int socket_address_information_length {};
};

#endif // CORE_TYPES_ENDPOINT_HPP

