#include "mutex.h"

#include <atomic>

namespace ct::sync {
// awaiters method impl
bool Mutex::MutexAwaiter::await_ready() noexcept {
  return enqueue_if_empty();
}

bool Mutex::MutexAwaiter::await_suspend(std::coroutine_handle<coro::Coroutine::promise_type> handle) {
  awaiting_coroutine = handle;
  if (enqueue_if_empty()) {
    return false;
  }
  mutex.queue.push(this);

  return true;
}

void Mutex::MutexAwaiter::await_resume() noexcept {}

Mutex::MutexAwaiter::MutexAwaiter(const Mutex& mutex) noexcept
    : mutex(mutex) {}

bool Mutex::MutexAwaiter::enqueue_if_empty() {
  return !mutex.locked.exchange(true);
}

Mutex::Mutex()
    : queue(new MutexAwaiter(*this))
    , gc(queue.get_dummy()) {}

bool Mutex::try_lock() noexcept {
  return MutexAwaiter{*this}.await_ready();
}

Mutex::MutexAwaiter& Mutex::lock() {
  MutexAwaiter* heaped_awaiter = new MutexAwaiter{*this};
  gc.push(heaped_awaiter);
  return *heaped_awaiter;
}

std::suspend_never Mutex::unlock() noexcept {
  MutexAwaiter* aw = queue.pop();
  if (!aw) {
    locked.store(false);
  } else {
    coro::Coroutine::current_scheduler()->spawn(aw->awaiting_coroutine.promise());
  }
  return {};
}

} // namespace ct::sync
