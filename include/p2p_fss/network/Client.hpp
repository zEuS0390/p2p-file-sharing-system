#ifndef P2P_FSS_NETWORK_CLIENT_HPP
#define P2P_FSS_NETWORK_CLIENT_HPP

#include "p2p_fss/network/ConnectionManager.hpp"

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

#endif // P2P_FSS_NETWORK_CLIENT_HPP
