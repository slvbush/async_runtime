#pragma once

#include "resumable.h"
#include "scheduler.h"

#include <condition_variable>
#include <cstddef>
#include <mutex>
#include <thread>
#include <vector>

namespace ct::sched {

// Just a thread-pool scheduler,
// not movable yet
class ThreadPool final : public IntrusiveListScheduler {
public:
  explicit ThreadPool(std::size_t threads);

  // Non-copyable
  ThreadPool(const ThreadPool&) = delete;
  ThreadPool& operator=(const ThreadPool&) = delete;

  // Non-moveable *yet*
  ThreadPool(ThreadPool&&) = delete;
  ThreadPool& operator=(ThreadPool&&) = delete;

  ~ThreadPool() override;

  void run();
  void spawn(Resumable<IntrusiveListScheduler>& task) final;

private:
  std::atomic<bool> _runs = false;
  std::mutex _mutex;
  std::condition_variable _cv;
  std::vector<std::jthread> _workers;
};

} // namespace ct::sched
