#ifndef CORE_NETWORK_SERVER_HPP
#define CORE_NETWORK_SERVER_HPP

#include <sys/socket.h>
#include <unordered_map>
#include <poll.h>
#include <vector>
#include <mutex>

#include "core/network/Socket.hpp"
#include "core/types/Endpoint.hpp"

class Server: public Socket
{
private:
  std::unordered_map<int, Endpoint> clients;
  std::vector<pollfd> client_pollfds;
  bool is_listening;
  bool is_monitoring;
public:
  std::mutex mutex;
  Server();
  ~Server();
  void startListening();
  void stopListening();
  void startMonitoring();
  void stopMonitoring();
  int getNumberOfClients();
};

#endif
