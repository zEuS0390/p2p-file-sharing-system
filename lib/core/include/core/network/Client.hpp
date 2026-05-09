#ifndef CORE_NETWORK_CLIENT_HPP
#define CORE_NETWORK_CLIENT_HPP

#include <string>
#include <vector>
#include "core/types/Endpoint.hpp"

class Client
{
private:
  std::vector<Endpoint> servers;
public:
  Client();
  ~Client();
  int connectToServer(const std::string&, int);
  int disconnectToServer(int);
  ssize_t sendMessage(int, const std::string&);
};

#endif
