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

// queue methods impl
Mutex::LockFreeAwaitersQueue::LockFreeAwaitersQueue(MutexAwaiter* dummy)
    : head(dummy)
    , tail(dummy) {}

Mutex::MutexAwaiter* Mutex::LockFreeAwaitersQueue::pop() {
  while (true) {
    MutexAwaiter* old_head = head;
    MutexAwaiter* old_tail = tail;
    MutexAwaiter* next_head = head.load()->next_in_queue;
    if (old_head == old_tail) {
      if (!next_head) {
        return nullptr;
      } else {
        tail.compare_exchange_strong(old_tail, next_head);
      }
    } else {
      if (head.compare_exchange_strong(old_head, next_head)) {
        return next_head;
      }
    }
  }
}

void Mutex::LockFreeAwaitersQueue::push(MutexAwaiter* awaiter) {
  while (true) {
    MutexAwaiter* old_tail = tail.load();
    MutexAwaiter* null_tail = nullptr;
    if (tail.load()->next_in_queue.compare_exchange_strong(null_tail, awaiter)) {
      tail.compare_exchange_strong(old_tail, awaiter);
      return;
    } else {
      tail.compare_exchange_strong(old_tail, tail.load()->next_in_queue.load());
    }
  }
}

Mutex::MutexAwaiter* Mutex::LockFreeAwaitersQueue::get_dummy() {
  return head.load(std::memory_order_relaxed);
}

// gc methods impl
Mutex::LockFreeGC::LockFreeGC(MutexAwaiter* dummy)
    : head(dummy) {}

void Mutex::LockFreeGC::push(MutexAwaiter* aw) {
  aw->next_in_gc = head.load();
  while (!head.compare_exchange_weak(aw->next_in_gc, aw))
    ;
}

void Mutex::LockFreeGC::clear() {
  MutexAwaiter* node = head.exchange(nullptr);
  while (node) {
    MutexAwaiter* to_delete = node;
    node = node->next_in_gc;
    delete to_delete;
  }
}

Mutex::LockFreeGC::~LockFreeGC() {
  clear();
}

// mutex methods impl
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
