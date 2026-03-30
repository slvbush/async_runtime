#pragma once

#include "resumable.h"
#include "scheduler.h"

#include <condition_variable>
#include <cstddef>
#include <mutex>
#include <thread>
#include <vector>

namespace ct::sched {
class ThreadPool final : public IntrusiveListScheduler {
public:
  explicit ThreadPool(std::size_t threads);

  // Non-copyable
  ThreadPool(const ThreadPool&) = delete;
  ThreadPool& operator=(const ThreadPool&) = delete;

  // Non-moveable
  ThreadPool(ThreadPool&&) = delete;
  ThreadPool& operator=(ThreadPool&&) = delete;

  ~ThreadPool() override;

  void run();
  void spawn(Resumable<IntrusiveListScheduler>& task) final;

private:
  std::atomic<bool> runs = false;
  std::mutex m;
  std::condition_variable cv;
  std::vector<std::jthread> workers;
};

} // namespace ct::sched
