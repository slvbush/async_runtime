#include "sync/mutex.h"

#include "coro/go.h"
#include "coro/yield.h"
#include "sched/run-loop.h"
#include "sync/event.h"

#include <gtest/gtest.h>

namespace ct_test {

using namespace ct;

TEST(MutexTest, Simple) {
  sched::RunLoop loop;

  sync::Mutex mutex;
  int cs = 0;

  coro::go(loop, [&](this auto) -> coro::Coroutine {
    co_await mutex.lock();
    ++cs;
    co_await mutex.unlock();

    co_await mutex.lock();
    ++cs;
    co_await mutex.unlock();
  });

  loop.run();

  ASSERT_EQ(cs, 2);
}

TEST(MutexTest, TryLock) {
  sched::RunLoop loop;

  sync::Mutex mutex;
  bool join = false;

  coro::go(loop, [&](this auto) -> coro::Coroutine {
    EXPECT_TRUE(mutex.try_lock());
    co_await mutex.unlock();

    co_await mutex.lock();
    co_await mutex.unlock();

    EXPECT_TRUE(mutex.try_lock());

    coro::go([&](this auto) -> coro::Coroutine {
      EXPECT_FALSE(mutex.try_lock());
      join = true;
      co_return;
    });

    while (!join) {
      co_await coro::yield();
    }

    co_await mutex.unlock();
  });

  loop.run();
}

TEST(MutexTest, CriticalSection) {
  constexpr int CS_NUM = 10;
  sched::RunLoop loop;

  sync::Mutex mutex;
  int cs = 0;

  coro::go(loop, [&](this auto) -> coro::Coroutine {
    for (int i = 0; i < CS_NUM; ++i) {
      co_await mutex.lock();
      ++cs;
      co_await mutex.unlock();
    }
  });

  loop.run();

  ASSERT_EQ(cs, CS_NUM);
}

TEST(MutexTest, Counter) {
  sched::RunLoop loop;

  sync::Mutex mutex;
  int cs = 0;

  constexpr int COROUTINES = 5;
  constexpr int CS_NUM = 5;

  for (int i = 0; i < COROUTINES; ++i) {
    coro::go(loop, [&](this auto) -> coro::Coroutine {
      for (int j = 0; j < CS_NUM; ++j) {
        co_await mutex.lock();
        ++cs;
        co_await mutex.unlock();
        co_await coro::yield();
      }
    });
  }

  loop.run();

  ASSERT_EQ(cs, COROUTINES * CS_NUM);
}

TEST(MutexTest, SuspendBehavior) {
  sched::RunLoop loop;

  sync::Mutex mutex;
  sync::Event ev;

  bool cs = false;

  coro::go(loop, [&](this auto) -> coro::Coroutine {
    co_await mutex.lock();
    co_await ev.wait();
    co_await mutex.unlock();
  });

  coro::go(loop, [&](this auto) -> coro::Coroutine {
    co_await mutex.lock();
    cs = true;
    co_await mutex.unlock();
  });

  // First run: task 2 cannot enter yet
  loop.run();
  ASSERT_FALSE(cs);

  // Signal event so task 1 releases lock
  ev.emit();

  // Second run handles both resumptions
  loop.run();
  ASSERT_TRUE(cs);
}

TEST(MutexTest, FIFOWaiters) {
  sched::RunLoop loop;

  sync::Mutex mutex;

  int next = 0;
  constexpr int WAITERS = 16;
  constexpr int YIELDS = 1024;

  coro::go(loop, [&](this auto) -> coro::Coroutine {
    co_await mutex.lock();
    for (int i = 0; i < YIELDS; ++i) {
      co_await coro::yield();
    }
    co_await mutex.unlock();
  });

  loop.run_at_most(1);

  for (int i = 0; i < WAITERS; ++i) {
    coro::go(loop, [&, i](this auto) -> coro::Coroutine {
      co_await mutex.lock();
      EXPECT_EQ(next, i); // FIFO guarantee
      next++;
      co_await mutex.unlock();
    });
    loop.run_at_most(1);
  }

  loop.run();

  ASSERT_EQ(next, WAITERS);
}

} // namespace ct_test
