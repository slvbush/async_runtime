#include "thread-pool.h"

namespace ct::sched {
ThreadPool::ThreadPool(std::size_t threads) {
  for (std::size_t i = 0; i < threads; ++i) {
    _workers.emplace_back([&](std::stop_token t) {
      while (true) {
        std::unique_lock ul(_mutex);
        _cv.wait(ul, [&] {
          return !_queue.empty() || t.stop_requested();
        });
        while (!_queue.empty()) {
          auto* task_ptr = std::addressof(_queue.front());
          _queue.pop_front();
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
  std::lock_guard guard(_mutex);
  for (auto& worker : _workers) {
    worker.request_stop();
  }
  _cv.notify_all();
}

void ThreadPool::run() {
  _runs.store(true, std::memory_order_release);
  _cv.notify_all();
}

void ThreadPool::spawn(Resumable<IntrusiveListScheduler>& task) {
  std::lock_guard guard(_mutex);
  _queue.push_back(task);
  if (_runs.load(std::memory_order_acquire)) {
    _cv.notify_one();
  }
}
} // namespace ct::sched
