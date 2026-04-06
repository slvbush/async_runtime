#include "yield.h"

#include "coroutine.h"

namespace ct::coro {

bool YieldAwaiter::await_ready() {
  return false;
}

void YieldAwaiter::await_suspend(std::coroutine_handle<coro::Coroutine::promise_type>) {
  Coroutine::current_scheduler()->spawn(Coroutine::current().promise());
}

void YieldAwaiter::await_resume() {}

YieldAwaiter yield() {
  return {};
}

} // namespace ct::coro
