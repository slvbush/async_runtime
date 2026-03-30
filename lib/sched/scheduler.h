#pragma once

#include "resumable.h"

namespace ct::sched {

class IntrusiveListScheduler {
public:
  // All tasks that were spawned must be eventually executed
  virtual void spawn(Resumable<IntrusiveListScheduler>&) = 0;
  virtual ~IntrusiveListScheduler() = default;

protected:
  ct::intrusive::List<Resumable<IntrusiveListScheduler>> queue;
};

} // namespace ct::sched
