#include "sched/run-loop.h"

#include "alloc-count.h"
#include "inline.h"
#include "lambda-resumable.h"
#include "sched/resumable.h"
#include "sched/scheduler.h"

#include <gtest/gtest.h>

#include <array>
#include <cstddef>
#include <cstdlib>
#include <utility>

namespace ct_test {

using namespace ct::sched;

class RunLoopTest : public ::testing::Test {
protected:
  static void sched_n(RunLoop& loop, std::size_t n) {
    if (n > 0) {
      go(loop, [&loop, n] {
        sched_n(loop, n - 1);
      });
    }
  }

  template <typename F>
  static void go(IntrusiveListScheduler& sched, F&& fn) {
    auto* t = new LambdaResumable(std::forward<F>(fn));
    sched.spawn(*t);
  }
};

TEST_F(RunLoopTest, Simple) {
  RunLoop loop;

  std::size_t step = 0;

  EXPECT_TRUE(loop.empty());
  EXPECT_FALSE(loop.run_next());
  EXPECT_EQ(loop.run_at_most(99), 0U);

  go(loop, [&] {
    step = 1;
  });

  EXPECT_FALSE(loop.empty());
  EXPECT_EQ(step, 0U);

  go(loop, [&] {
    step = 2;
  });

  EXPECT_EQ(step, 0U);

  EXPECT_TRUE(loop.run_next());
  EXPECT_EQ(step, 1U);

  EXPECT_FALSE(loop.empty());

  go(loop, [&] {
    step = 3;
  });

  EXPECT_EQ(loop.run_at_most(99), 2U);
  EXPECT_EQ(step, 3U);

  EXPECT_TRUE(loop.empty());
  EXPECT_FALSE(loop.run_next());
}

TEST_F(RunLoopTest, Empty) {
  RunLoop loop;

  EXPECT_TRUE(loop.empty());
  EXPECT_FALSE(loop.run_next());
  EXPECT_EQ(loop.run_at_most(7), 0U);
  EXPECT_EQ(loop.run(), 0U);
}

TEST_F(RunLoopTest, RunAtMost) {
  const std::size_t ITERATIONS = 256;
  const std::size_t STEP = 7;

  RunLoop loop;

  sched_n(loop, ITERATIONS);

  std::size_t tasks = 0;
  while (!loop.empty()) {
    tasks += loop.run_at_most(STEP);
  }

  EXPECT_EQ(tasks, ITERATIONS);
}

TEST_F(RunLoopTest, RunAtMostNewTasks) {
  RunLoop loop;

  go(loop, [&] {
    go(loop, [] {});
  });

  EXPECT_EQ(loop.run_at_most(2), 2U);
}

TEST_F(RunLoopTest, Run) {
  const std::size_t ITERATIONS = 256;
  RunLoop loop;

  sched_n(loop, ITERATIONS);

  EXPECT_EQ(loop.run(), ITERATIONS);
}

TEST_F(RunLoopTest, RunTwice) {
  const std::size_t ITERATIONS_1 = 11;
  const std::size_t ITERATIONS_2 = 7;

  RunLoop loop;

  sched_n(loop, ITERATIONS_1);
  EXPECT_EQ(loop.run(), ITERATIONS_1);

  sched_n(loop, ITERATIONS_2);
  EXPECT_EQ(loop.run(), ITERATIONS_2);
}

TEST_F(RunLoopTest, NoAllocations) {
  const std::size_t RESUMABLES = 100;
  RunLoop loop;
  auto count = g_allocation_count.load();

  struct StackResumable : Resumable<IntrusiveListScheduler> {
    void resume(IntrusiveListScheduler& /*unused*/) noexcept final {
      resumed = true;
    }

    bool resumed = false;
  };

  std::array<StackResumable, RESUMABLES> resumables;

  for (auto& resumable : resumables) {
    loop.spawn(resumable);
  }

  EXPECT_EQ(loop.run(), 100);

  for (const auto& resumable : resumables) {
    EXPECT_TRUE(resumable.resumed);
  }

  EXPECT_EQ(g_allocation_count.load(), count);
}

TEST_F(RunLoopTest, RescheduleBetweenSchedulers) {
  RunLoop loop;

  int state = 0;

  struct DualResumable
      : public Resumable<IntrusiveListScheduler>
      , public Resumable<InlineScheduler> {
    explicit DualResumable(int& s)
        : state(s) {}

    void resume(IntrusiveListScheduler& /*unused*/) noexcept final {
      state = 1;
    }

    void resume(InlineScheduler& /*unused*/) noexcept final {
      state = 2;
    }

    int& state; // NOLINT(cppcoreguidelines-avoid-const-or-ref-data-members)
  };

  DualResumable resumable{state};

  InlineScheduler::instance().spawn(resumable);

  EXPECT_EQ(state, 2);
  EXPECT_TRUE(loop.empty());
  loop.spawn(resumable);

  EXPECT_TRUE(loop.run_next());
  EXPECT_EQ(state, 1);
}

} // namespace ct_test
