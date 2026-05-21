#ifndef CORE_NETWORK_SERVER_HPP
#define CORE_NETWORK_SERVER_HPP

#include <sys/socket.h>
#include <unordered_map>
#include <poll.h>
#include <vector>
#include <mutex>

#include "core/network/Socket.hpp"
#include "core/types/ClientConnection.hpp"

class Server: public Socket
{
protected:
  std::unordered_map<int, ClientConnection> clients;
  std::vector<pollfd> client_pollfds;
private:
  bool is_listening;
  bool is_monitoring;
  unsigned int listen_limit;
  std::mutex mutex;
public:
  Server();
  ~Server();
  void startListening();
  void stopListening();
  void startMonitoring();
  void stopMonitoring();
  void parseMessage(ClientConnection*);
  int getNumberOfClients();
};

#endif
