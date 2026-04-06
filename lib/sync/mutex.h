#pragma once

#include "../coro/coroutine.h"
#include "../utils/lf-intrusive-queue.h"
#include "../utils/lf-intrusive-stack.h"

#include <queue>

#include <atomic>
#include <coroutine>
#include <memory>
#include <mutex>

namespace ct::sync {

// Must preserve FIFO ordering

class Mutex {
private:
  class MutexAwaiter : public ct::intrusive::AtomicSingleLinkedListElementImpl,
                       public ct::intrusive::SingleLinkedListElementImpl {
  public:
    bool await_ready() noexcept;
    bool
    await_suspend(std::coroutine_handle<coro::Coroutine::promise_type> handle);
    void await_resume() noexcept;

  private:
    friend class Mutex;

    MutexAwaiter(const Mutex &mutex) noexcept;

    bool enqueue_if_empty();

    const Mutex &mutex;
    std::coroutine_handle<coro::Coroutine::promise_type> awaiting_coroutine;
  };

public:
  Mutex();
  bool try_lock() noexcept;
  MutexAwaiter &lock();
  std::suspend_never unlock() noexcept;

private:
  mutable std::atomic<bool> locked{false};
  mutable ct::intrusive::LockFreeIntrusiveQueue<MutexAwaiter> queue;
  mutable ct::intrusive::LockFreeIntrusiveStack<MutexAwaiter> gc;
};

} // namespace ct::sync
