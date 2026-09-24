#ifndef CORE_NETWORK_SERVER_CONNECTION_MANAGER_HPP
#define CORE_NETWORK_SERVER_CONNECTION_MANAGER_HPP

#include <atomic>
#include <vector>

#include "core/network/ConnectionManager.hpp"
#include "core/network/IMessageHandler.hpp"

/*
 * Manages the connection with the clients.
 */
class ServerConnectionManager: public ConnectionManager
{
private:
  std::vector<int> m_listening_socket_descriptors;
  std::atomic<bool> m_is_listening;
public:
  explicit ServerConnectionManager(IMessageHandler&);
  void startAcceptConnectionLoop();
  void stopAcceptConnectionLoop();
  void initServer(uint16_t);
};

#endif // CORE_NETWORK_SERVER_CONNECTION_MANAGER_HPP

