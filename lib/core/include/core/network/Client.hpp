#ifndef CORE_NETWORK_CLIENT_HPP
#define CORE_NETWORK_CLIENT_HPP

#include <unordered_map>
#include <string>

#include "core/types/Endpoint.hpp"

class Client
{
protected:
  std::unordered_map<int, Endpoint> servers;
public:
  Client();
  ~Client();
  int connectToServer(const std::string&, int);
  int disconnectToServer(int);
  ssize_t sendAll(int, const char*, size_t);
};

#endif
