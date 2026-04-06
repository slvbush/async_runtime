#include "wait-group.h"

namespace ct_test {

void WaitGroup::add(std::size_t count) {
  std::lock_guard guard{_lock};
  _work += count;
}

void WaitGroup::done() {
  std::lock_guard guard{_lock};
  if (--_work == 0) {
    _zero_work_cv.notify_all();
  }
}

void WaitGroup::wait() {
  std::unique_lock guard{_lock};
  _zero_work_cv.wait(guard, [this] {
    return _work == 0;
  });
}

} // namespace ct_test
