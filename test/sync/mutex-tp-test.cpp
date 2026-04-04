
#include "coro/go.h"
#include "coro/yield.h"
#include "sched/thread-pool.h"
#include "sync/mutex.h"
#include "wait-group.h"

#include <gtest/gtest.h>

namespace ct_test {

using namespace ct;

TEST(MutexThreadPoolTest, CriticalSection) {
  constexpr int CS_NUM = 1'000'000;
  int cs = 0;

  {
    sched::ThreadPool pool{4};
    sync::Mutex mutex;
    WaitGroup wg;

    wg.add(1 + CS_NUM);
    coro::go(pool, [&](this auto) -> coro::Coroutine {
      for (int j = 0; j < CS_NUM; ++j) {
        coro::go(pool, [&](this auto) -> coro::Coroutine {
          co_await mutex.lock();
          ++cs;
          co_await mutex.unlock();
          wg.done();
        });
      }
      wg.done();
      co_return;
    });

    pool.run();
    wg.wait();
  }

  assert(cs == CS_NUM);
}

TEST(MutexThreadPoolTest, TryLockStress) {
  constexpr int CS_NUM = 1'000'000;
  int cs = 0;

  sched::ThreadPool pool{4};
  sync::Mutex mutex;
  WaitGroup wg;

  wg.add(1);
  coro::go(pool, [&](this auto) -> coro::Coroutine {
    for (int i = 0; i < CS_NUM; ++i) {
      bool locked = false;
      while (!locked) {
        locked = mutex.try_lock();
        if (locked) {
          ++cs;
          co_await mutex.unlock();
        } else {
          co_await coro::yield();
        }
      }
    }
    wg.done();
  });

  pool.run();
  wg.wait();

  ASSERT_EQ(cs, CS_NUM);
}

TEST(MutexThreadPoolTest, YieldStress) {
  constexpr int CS_NUM = 100'000;
  constexpr int COROUTINES = 8;
  int cs = 0;

  sched::ThreadPool pool{4};
  sync::Mutex mutex;
  WaitGroup wg;

  wg.add(COROUTINES);
  for (int i = 0; i < COROUTINES; ++i) {
    coro::go(pool, [&](this auto) -> coro::Coroutine {
      for (int j = 0; j < CS_NUM; ++j) {
        co_await mutex.lock();
        ++cs;
        co_await coro::yield(); // yield in critical section.. not the best idea
        co_await mutex.unlock();
        co_await coro::yield();
      }
      wg.done();
    });
  }

  pool.run();
  wg.wait();

  ASSERT_EQ(cs, 8 * CS_NUM);
}

} // namespace ct_test
