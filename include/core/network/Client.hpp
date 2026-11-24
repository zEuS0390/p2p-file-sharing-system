#ifndef CORE_NETWORK_CLIENT_HPP
#define CORE_NETWORK_CLIENT_HPP

#include "core/network/ConnectionManager.hpp"

class Client: protected ConnectionManager
{
public:
  Client();
  ~Client();
private:
  void initListeners(std::uint16_t port);
  void startListening();
  void stopListening();
};

#endif // CORE_NETWORK_CONNECTION_MANAGER_HPP
