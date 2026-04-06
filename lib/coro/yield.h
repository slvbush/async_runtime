#pragma once

#include "coroutine.h"

#include <coroutine>

namespace ct::coro {

// Returns an execution flow to a scheduler
// resuming coroutine
class YieldAwaiter {
public:
  bool await_ready();
  void await_suspend(std::coroutine_handle<coro::Coroutine::promise_type>);
  void await_resume();
};

YieldAwaiter yield();

} // namespace ct::coro
