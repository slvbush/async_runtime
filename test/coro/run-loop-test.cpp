#include "sched/run-loop.h"

#include "coro/coroutine.h"
#include "coro/go.h"
#include "coro/yield.h"

#include <gtest/gtest.h>

#include <vector>

namespace ct_test {

using namespace ct::sched;
using namespace ct::coro;

class CoroutineRunLoopTest : public ::testing::Test {
public:
  RunLoop loop;
};

TEST_F(CoroutineRunLoopTest, YieldSingle) {
  bool done = false;

  go(loop, [&](this auto) -> Coroutine {
    done = true;
    co_return;
  });

  EXPECT_FALSE(done);
  EXPECT_EQ(loop.run_at_most(1), 1U); // run the first task
  EXPECT_TRUE(done);
}

TEST_F(CoroutineRunLoopTest, YieldMultiple) {
  RunLoop loop;

  const size_t COROUTINES = 7;
  size_t finished = 0;

  for (size_t i = 0; i < COROUTINES; ++i) {
    go(loop, [&](this auto) -> Coroutine {
      ++finished;
      co_return;
    });
  }

  EXPECT_EQ(loop.run(), COROUTINES);
  EXPECT_EQ(finished, COROUTINES);
}

TEST_F(CoroutineRunLoopTest, FireChild) {
  RunLoop loop;

  int counter = 0;

  go(loop, [&](this auto) -> Coroutine {
    go([&](this auto) -> Coroutine {
      counter++;
      co_return;
    });
    co_return;
  });

  EXPECT_TRUE(loop.run_next()); // run parent first step
  EXPECT_EQ(counter, 0);
  EXPECT_EQ(loop.run(), 1U); // run child
  EXPECT_EQ(counter, 1);
}

TEST_F(CoroutineRunLoopTest, PingPongYield) {
  RunLoop loop;

  unsigned int turn = 0;

  go(loop, [&](this auto) -> Coroutine {
    for (size_t i = 0; i < 3; ++i) {
      EXPECT_EQ(turn, 0);
      turn ^= 1;
      co_await yield();
    }
  });

  go(loop, [&](this auto) -> Coroutine {
    for (size_t i = 0; i < 3; ++i) {
      EXPECT_EQ(turn, 1);
      turn ^= 1;
      co_await yield();
    }
  });

  loop.run();
  EXPECT_EQ(turn, 0); // final state
}

TEST_F(CoroutineRunLoopTest, YieldGroup) {
  RunLoop loop;

  const size_t COROUTINES = 3;
  const size_t YIELDS = 4;
  size_t finished = 0;

  for (size_t i = 0; i < COROUTINES; ++i) {
    go(loop, [&](this auto) -> Coroutine {
      for (size_t k = 0; k < YIELDS; ++k) {
        co_await yield();
      }
      ++finished;
    });
  }

  EXPECT_EQ(loop.run(), COROUTINES * (YIELDS + 1));
  EXPECT_EQ(finished, COROUTINES);
}

TEST_F(CoroutineRunLoopTest, FIFO) {
  std::vector<int> order;

  auto make_co = [&](int id) -> Coroutine {
    order.push_back(id);
    co_return;
  };

  go(loop, [&](this auto) -> Coroutine {
    return make_co(1);
  });
  go(loop, [&](this auto) -> Coroutine {
    return make_co(2);
  });
  go(loop, [&](this auto) -> Coroutine {
    return make_co(3);
  });

  loop.run();

  std::vector<int> expected{1, 2, 3};
  EXPECT_EQ(order, expected);
}

namespace {

bool g_flag; // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)

Coroutine function_coro() {
  g_flag = true;
  co_return;
}

} // namespace

TEST_F(CoroutineRunLoopTest, FunctionCoroutine) {
  go(loop, function_coro);

  EXPECT_FALSE(g_flag);
  loop.run();
  EXPECT_TRUE(g_flag);
}

} // namespace ct_test
