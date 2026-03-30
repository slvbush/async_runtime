#include "coroutine.h"

namespace ct::coro {

Coroutine Coroutine::PromiseType::get_return_object() {
  return Coroutine(std::coroutine_handle<PromiseType>::from_promise(*this));
}

std::suspend_always Coroutine::PromiseType::initial_suspend() noexcept {
  return {};
}

bool Coroutine::PromiseType::FinalSuspendAwaiter::await_ready() noexcept {
  return false;
}

void Coroutine::PromiseType::FinalSuspendAwaiter::await_suspend(
    std::coroutine_handle<Coroutine::promise_type> h
) noexcept {
  h.destroy();
}

void Coroutine::PromiseType::FinalSuspendAwaiter::await_resume() noexcept {}

Coroutine::PromiseType::FinalSuspendAwaiter Coroutine::PromiseType::final_suspend() noexcept {
  return {};
}

void Coroutine::PromiseType::return_void() {}

void Coroutine::PromiseType::unhandled_exception() {
  std::terminate();
}

void Coroutine::PromiseType::resume(sched::IntrusiveListScheduler& scheduler) noexcept {
  current_sched = &scheduler;
  current_coro = Coroutine(std::coroutine_handle<Coroutine::PromiseType>::from_promise(*this));
  handle_t::from_promise(*this).resume();
}

Coroutine::Coroutine(std::coroutine_handle<promise_type> h)
    : _handle(std::move(h)) {}

Coroutine::Coroutine() = default;

Coroutine::~Coroutine() {}

Coroutine::promise_type& Coroutine::promise() {
  return _handle.promise();
}

Coroutine& Coroutine::current() {
  return current_coro;
}

sched::IntrusiveListScheduler* Coroutine::current_scheduler() {
  return current_sched;
}

thread_local sched::IntrusiveListScheduler* Coroutine::current_sched = nullptr;
thread_local Coroutine Coroutine::current_coro;

} // namespace ct::coro
