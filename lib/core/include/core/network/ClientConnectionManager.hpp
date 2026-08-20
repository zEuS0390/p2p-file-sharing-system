#ifndef CORE_NETWORK_CLIENT_CONNECTION_MANAGER_HPP
#define CORE_NETWORK_CLIENT_CONNECTION_MANAGER_HPP

#include "core/network/ConnectionManager.hpp"
#include "core/network/IMessageHandler.hpp"

/*
 * Manages the connection with the server.
 */
class ClientConnectionManager: public ConnectionManager
{
public:
  explicit ClientConnectionManager(IMessageHandler&);
};

#endif

