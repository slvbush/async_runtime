#include "event.h"

namespace ct::sync {
Event::EventAwaiter::EventAwaiter(const Event& event) noexcept
    : event(event) {}

bool Event::EventAwaiter::await_ready() const noexcept {
  return event.emitted();
}

bool Event::EventAwaiter::await_suspend(std::coroutine_handle<coro::Coroutine::promise_type> handle) noexcept {
  const void* const set_state = &event;

  awaiting_coroutine = handle;

  void* old_value = event.state;
  do {
    if (old_value == set_state) {
      return false;
    }

    next = static_cast<EventAwaiter*>(old_value);
  } while (!event.state.compare_exchange_strong(old_value, this));

  return true;
}

void Event::EventAwaiter::await_resume() noexcept {}

void Event::emit() noexcept {
  void* old_value = state.exchange(this);
  if (old_value != this) {
    auto* waiters = static_cast<EventAwaiter*>(old_value);
    while (waiters) {
      auto* next = waiters->next;
      coro::Coroutine::current_scheduler()->spawn(waiters->awaiting_coroutine.promise());
      waiters = next;
    }
  }
}

bool Event::emitted() const noexcept {
  return state == this;
}

Event::EventAwaiter Event::wait() noexcept {
  return EventAwaiter{*this};
}
} // namespace ct::sync
