#include "run-loop.h"

namespace ct::sched {
RunLoop::RunLoop() = default;

std::size_t RunLoop::run_at_most(std::size_t limit) {
  std::size_t cnt = 0;
  while (cnt != limit && !empty()) {
    run_next();
    ++cnt;
  }
  return cnt;
}

bool RunLoop::run_next() {
  if (!empty()) {
    auto& task_ptr = _queue.front();
    _queue.pop_front();
    task_ptr.resume(*this);
    return true;
  }
  return false;
}

std::size_t RunLoop::run() {
  return run_at_most(std::numeric_limits<std::size_t>::max());
}

void RunLoop::spawn(Resumable<IntrusiveListScheduler>& task) {
  _queue.push_back(task);
}

bool RunLoop::empty() const noexcept {
  return _queue.empty();
}

std::size_t RunLoop::size() const noexcept {
  return _queue.size();
}
} // namespace ct::sched
