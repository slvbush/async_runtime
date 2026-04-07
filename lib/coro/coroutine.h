#pragma once

#include "../sched/resumable.h"
#include "../sched/scheduler.h"

#include <coroutine>
#include <exception>

namespace ct::coro {

// Basically, an async task-class, supports only yielding
// inside a coroutine body, lifetime is managed by the promise-type
class Coroutine {
  class PromiseType : public sched::Resumable<sched::IntrusiveListScheduler> {
  public:
    using handle_t = std::coroutine_handle<PromiseType>;
    Coroutine get_return_object();

    std::suspend_always initial_suspend() noexcept;

    class FinalSuspendAwaiter {
    public:
      bool await_ready() noexcept;
      void await_suspend(std::coroutine_handle<PromiseType> h) noexcept;
      void await_resume() noexcept;
    };

    FinalSuspendAwaiter final_suspend() noexcept;
    void return_void();
    void unhandled_exception();

    void resume(sched::IntrusiveListScheduler& scheduler) noexcept override;
  };

public:
  using promise_type = PromiseType;

  explicit Coroutine(std::coroutine_handle<promise_type> h);
  Coroutine();

  // Non-copyable
  Coroutine(const Coroutine&) = delete;
  Coroutine& operator=(const Coroutine&) = delete;

  // Moveable
  Coroutine(Coroutine&& other) noexcept = default;
  Coroutine& operator=(Coroutine&& other) noexcept = default;

public:
  promise_type& promise();
  static Coroutine& current();
  static sched::IntrusiveListScheduler* current_scheduler();

private:
  static thread_local Coroutine _current_coro;
  static thread_local sched::IntrusiveListScheduler* _current_sched;
  std::coroutine_handle<promise_type> _handle;
};

} // namespace ct::coro
