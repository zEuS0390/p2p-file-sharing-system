#ifndef CORE_NETWORK_IOMULTIPLEXER_HPP
#define CORE_NETWORK_IOMULTIPLEXER_HPP

#include <cstring>
#include <iostream>
#include <unistd.h>
#include <sys/epoll.h>
#include <atomic>
#include <array>

struct IOMultiplexer
{
  std::atomic<int> m_epfd;
  static constexpr int MAX_EVENTS {64};
  std::array<epoll_event, MAX_EVENTS> m_epoll_events;
  IOMultiplexer();
  ~IOMultiplexer();
  int addFD(int fd) const;
  void readFD(int fd, const void* buffer, std::size_t buffer_size) const;
  void writeFD(int fd, const void* buffer, std::size_t buffer_size) const;
  void removeFD(int fd) const;
  int modifyFDSettings(int fd, epoll_event& ev) const;
  template <typename Func, typename = std::enable_if_t<std::is_invocable_v<Func, int, epoll_event&> > >
  void monitorEvents(Func&& func);
};

template <typename Func, typename>
void IOMultiplexer::monitorEvents(Func&& func)
{
  int ready {epoll_wait(m_epfd,m_epoll_events.data(),MAX_EVENTS,-1)};
  if (ready == -1)
  {
    if (errno == EINTR)
    {
      std::cout << std::strerror(errno) << std::endl;
      return;
    }
    std::cerr << "epoll_wait: ready = -1" << std::endl;
    return;
  }
  for (int i {0}; i < ready; ++i)
  {
    epoll_event& event {m_epoll_events.at(i)};
    int fd {event.data.fd};
    std::forward<Func>(func)(fd, event);
  }
}

#endif // CORE_NETWORK_IOMULTIPLEXER_HPP

