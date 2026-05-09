#ifndef CORE_NETWORK_CLIENT_HPP
#define CORE_NETWORK_CLIENT_HPP

#include <string>
#include <vector>
#include "types/Endpoint.hpp"

class Client
{
private:
  std::vector<Endpoint> servers;
public:
  Client();
  ~Client();
  int connectToServer(const std::string&, int);
  int disconnectToServer(int);
};

#endif
