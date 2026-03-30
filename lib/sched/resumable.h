#pragma once

#include "intrusive-list.h"

namespace ct::sched {

template <typename Scheduler>
class Resumable : public ct::intrusive::ListElement<> {
public:
  virtual void resume(Scheduler&) noexcept = 0;
  virtual ~Resumable() = default;
};

} // namespace ct::sched
