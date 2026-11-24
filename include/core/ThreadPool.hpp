#ifndef CORE_THREAD_POOL_HEADER
#define CORE_THREAD_POOL_HEADER

#include <condition_variable>
#include <type_traits>
#include <functional>
#include <utility>
#include <future>
#include <thread>
#include <vector>
#include <mutex>
#include <queue>

class ThreadPool
{
public:
  ThreadPool(std::size_t n_threads = std::thread::hardware_concurrency());
  ~ThreadPool();
  template <typename Func, typename... Args>
  auto execute(Func&& func, Args&&... args)
    -> std::future<std::invoke_result_t<Func, Args...> >
  {
    using return_type = typename std::invoke_result<Func&&, Args&&...>::type;
    auto bound_func = std::bind(std::forward<Func>(func), std::forward<Args>(args)...);
    auto task = std::make_shared<std::packaged_task<return_type()> >(std::move(bound_func));
    auto future = task->get_future();
    {
      std::lock_guard<std::mutex> lock(m_mutex);
      m_tasks.emplace([task]{ (*task)(); });
    }
    m_cv.notify_one();
    return future;
  }
private:
  std::vector<std::thread> m_workers;
  std::queue<std::function<void()> > m_tasks;
  std::condition_variable m_cv;
  std::mutex m_mutex;
  bool m_stop;
};

#endif
