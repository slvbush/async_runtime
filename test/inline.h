#pragma once

#include "sched/resumable.h"

namespace ct_test {

class InlineScheduler {
  InlineScheduler() = default;

public:
  void spawn(ct::sched::Resumable<InlineScheduler>& resumable) {
    resumable.resume(*this);
  }

  static InlineScheduler& instance() {
    static InlineScheduler sched;
    return sched;
  }
};

} // namespace ct_test
