#include <cstring>
#include <iostream>
#include <stdexcept>
#include <sys/epoll.h>
#include <unistd.h>

#include "core/network/IOMultiplexer.hpp"

IOMultiplexer::IOMultiplexer():
  m_epfd{-1}
{
  m_epfd.store(epoll_create1(0));
  if (m_epfd.load() == -1)
    throw std::runtime_error("couldn't create epoll instance.");
}

IOMultiplexer::~IOMultiplexer()
{
  close(m_epfd);
}

int IOMultiplexer::addFD(int fd) const
{
  epoll_event ep_event {};
  ep_event.events = EPOLLIN;
  ep_event.data.fd = fd;
  if (epoll_ctl(m_epfd, EPOLL_CTL_ADD, fd, &ep_event) == -1)
  {
    close(fd);
    std::cout << "eventfd: couldn't add fd to epoll." << std::endl;
    return -1;
  }
  return 0;
}

void IOMultiplexer::readFD(int fd, const void *buffer, std::size_t buffer_size) const
{
  if (read(fd, &buffer, sizeof(buffer_size)) == -1)
  {
    if (errno != EAGAIN && errno != EWOULDBLOCK)
      std::cerr << "read command fd" << std::endl;
  }
}

void IOMultiplexer::writeFD(int fd, const void *buffer, std::size_t buffer_size) const
{
  if (write(fd, &buffer, sizeof(buffer_size)) == -1)
  {
    if (errno != EAGAIN && errno != EWOULDBLOCK)
      std::cerr << "write command fd" << std::endl;
  }
}

void IOMultiplexer::removeFD(int fd) const
{
  epoll_ctl(m_epfd, EPOLL_CTL_DEL, fd, nullptr);
  close(fd);
}

int IOMultiplexer::modifyFDSettings(int fd, epoll_event& ev) const
{
  if (epoll_ctl(m_epfd, EPOLL_CTL_MOD, fd, &ev) == -1)
  {
    std::cout << "eventfd: couldn't modify fd settings to epoll." << std::endl;
    close(fd);
    return -1;
  }
  return 0;
}

