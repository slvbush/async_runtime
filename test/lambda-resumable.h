#pragma once

#include "sched/resumable.h"
#include "sched/scheduler.h"

#include <utility>

namespace ct_test {

template <typename F>
class LambdaResumable : public ct::sched::Resumable<ct::sched::IntrusiveListScheduler> {
public:
  explicit LambdaResumable(F&& fn) // NOLINT(cppcoreguidelines-rvalue-reference-param-not-moved)
      : _fn(std::forward<F>(fn)) {}

  void resume(ct::sched::IntrusiveListScheduler& /*unused*/) noexcept final {
    _fn();
    delete this;
  }

private:
  std::decay_t<F> _fn;
};

template <typename F>
LambdaResumable(F&&) -> LambdaResumable<F>;

} // namespace ct_test
