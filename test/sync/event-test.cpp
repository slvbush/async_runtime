
#include "sync/event.h"

#include "coro/go.h"
#include "coro/yield.h"
#include "sched/run-loop.h"
#include "sched/thread-pool.h"

#include <gtest/gtest.h>

namespace ct_test {

using namespace ct;

TEST(EventTest, ConsumerProducer) {
  sched::RunLoop loop;

  static const std::string MESSAGE = "Hello world!";

  sync::Event event;
  std::string data;
  bool ok = false;

  coro::go(loop, [&](this auto) -> coro::Coroutine {
    co_await event.wait();
    EXPECT_EQ(data, MESSAGE);
    ok = true;
  });

  coro::go(loop, [&](this auto) -> coro::Coroutine {
    data = MESSAGE;
    event.emit();
    co_return;
  });

  loop.run();

  ASSERT_TRUE(ok);
}

TEST(EventTest, MultipleConsumers) {
  sched::RunLoop loop;

  sync::Event event;
  int work = 0;
  size_t waiters = 0;

  static const size_t CONSUMERS = 7;

  for (size_t i = 0; i < CONSUMERS; ++i) {
    coro::go(loop, [&](this auto) -> coro::Coroutine {
      co_await event.wait();
      EXPECT_EQ(work, 1);
      ++waiters;
    });
    loop.run_at_most(1);
  }

  coro::go(loop, [&](this auto) -> coro::Coroutine {
    ++work;
    event.emit();
    co_return;
  });

  loop.run();

  ASSERT_EQ(waiters, CONSUMERS);
}

TEST(EventTest, SuspendBehavior) {
  sched::RunLoop loop;

  sync::Event event;
  bool ok = false;

  coro::go(loop, [&](this auto) -> coro::Coroutine {
    co_await event.wait();
    ok = true;
  });

  {
    size_t tasks = loop.run();
    ASSERT_LE(tasks, 1);
  }

  coro::go(loop, [&](this auto) -> coro::Coroutine {
    co_return event.emit();
  });

  loop.run();

  ASSERT_TRUE(ok);
}

} // namespace ct_test
