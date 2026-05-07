#ifndef CORE_NETWORK_SOCKET_HPP
#define CORE_NETWORK_SOCKET_HPP

class Socket
{
protected:
  int descriptor;
public:
  Socket();
  int getDescriptor() const;
};

#endif
