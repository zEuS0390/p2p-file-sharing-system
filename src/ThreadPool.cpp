#include <functional>
#include <cstddef>

#include "core/ThreadPool.hpp"

ThreadPool::ThreadPool(std::size_t n_threads):
  m_stop{false}
{
  for (std::size_t i {0}; i < n_threads; ++i)
  {
    m_workers.emplace_back(
      [this]
      {
        while (true)
        {
          std::function<void()> task;
          {
            std::unique_lock<std::mutex> lock(m_mutex);
            m_cv.wait(
              lock,
              [this]() { return m_stop || !m_tasks.empty(); }
            );
            if (m_stop && m_tasks.empty())
              return;
            task = std::move(m_tasks.front());
            m_tasks.pop();
          }
          task();
        }
      }
    );
  }
}

ThreadPool::~ThreadPool()
{
  {
    std::unique_lock<std::mutex> lock(m_mutex);
    m_stop = true;
  }
  m_cv.notify_all();
  for (std::thread& worker: m_workers)
    if (worker.joinable())
      worker.join();
}
