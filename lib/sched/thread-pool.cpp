#include "thread-pool.h"

namespace ct::sched {
ThreadPool::ThreadPool(std::size_t threads) {
  for (std::size_t i = 0; i < threads; ++i) {
    workers.emplace_back([&](std::stop_token t) {
      while (true) {
        std::unique_lock ul(m);
        cv.wait(ul, [&] {
          return !queue.empty() || t.stop_requested();
        });
        while (!queue.empty()) {
          auto* task_ptr = std::addressof(queue.front());
          queue.pop_front();
          ul.unlock();
          task_ptr->resume(*this);
          ul.lock();
        }
        if (t.stop_requested()) {
          break;
        }
      }
    });
  }
}

ThreadPool::~ThreadPool() {
  std::lock_guard guard(m);
  for (auto& worker : workers) {
    worker.request_stop();
  }
  cv.notify_all();
}

void ThreadPool::run() {
  runs.store(true, std::memory_order_release);
  cv.notify_all();
}

void ThreadPool::spawn(Resumable<IntrusiveListScheduler>& task) {
  std::lock_guard guard(m);
  queue.push_back(task);
  if (runs.load(std::memory_order_acquire)) {
    cv.notify_one();
  }
}
} // namespace ct::sched
