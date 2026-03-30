#pragma once

#include "coro/coroutine.h"

#include <queue>

#include <atomic>
#include <coroutine>
#include <memory>
#include <mutex>

namespace ct::sync {

// Must preserve FIFO ordering

class Mutex {
private:
  class MutexAwaiter {
  public:
    bool await_ready() noexcept;
    bool await_suspend(std::coroutine_handle<coro::Coroutine::promise_type> handle);
    void await_resume() noexcept;

  private:
    friend class Mutex;

    MutexAwaiter(const Mutex& mutex) noexcept;

    bool enqueue_if_empty();

    const Mutex& mutex;
    std::coroutine_handle<coro::Coroutine::promise_type> awaiting_coroutine;
    std::atomic<MutexAwaiter*> next_in_queue{nullptr};
    MutexAwaiter* next_in_gc{nullptr}; // i saw these 2 node fields and wanted to make inheritance with different tags
    // but it turned out really bad-looking, so i decided to leave it like that
  };

  class LockFreeAwaitersQueue {
    std::atomic<MutexAwaiter*> head;
    std::atomic<MutexAwaiter*> tail;

  public:
    MutexAwaiter* pop();
    void push(MutexAwaiter* awaiter);

  private:
    friend class Mutex;

    LockFreeAwaitersQueue(MutexAwaiter* dummy);

    MutexAwaiter* get_dummy();
  };

  class LockFreeGC {
    std::atomic<MutexAwaiter*> head;

  public:
    void push(MutexAwaiter* aw);

  private:
    friend class Mutex;

    // these methods are private because only Mutex dtor can call them *not concurrently*
    LockFreeGC(MutexAwaiter* dummy);

    void clear();

    ~LockFreeGC();
  };

public:
  Mutex();
  bool try_lock() noexcept;
  MutexAwaiter& lock();
  std::suspend_never unlock() noexcept;

private:
  mutable std::atomic<bool> locked{false};
  mutable LockFreeAwaitersQueue queue;
  mutable LockFreeGC gc;
};

} // namespace ct::sync
