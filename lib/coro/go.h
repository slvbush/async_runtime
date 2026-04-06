#pragma once

#include "coroutine.h"

namespace ct::coro {

// Interface for placing a coroutine in scheduler
// 2nd overload is for inner coroutines
template <typename Scheduler, typename Routine>
void go(Scheduler& scheduler, const Routine& routine) {
  Coroutine::current() = routine();
  scheduler.spawn(Coroutine::current().promise());
}

template <typename Routine>
void go(const Routine& routine) {
  Coroutine::current_scheduler()->spawn(routine().promise());
}

} // namespace ct::coro
