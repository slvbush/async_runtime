#pragma once

#include "coro/coroutine.h"

#include <atomic>
#include <coroutine>

namespace ct::sync {

class Event {
private:
  class EventAwaiter {
  public:
    bool await_ready() const noexcept;
    bool await_suspend(std::coroutine_handle<coro::Coroutine::promise_type> handle) noexcept;
    void await_resume() noexcept;

  private:
    friend class Event;

    EventAwaiter(const Event& event) noexcept;

    std::coroutine_handle<coro::Coroutine::promise_type> awaiting_coroutine;
    EventAwaiter* next;
    const Event& event;
  };

public:
  void emit() noexcept;
  bool emitted() const noexcept;
  EventAwaiter wait() noexcept;

private:
  friend struct awaiter;
  mutable std::atomic<void*> state{nullptr};
};

} // namespace ct::sync
