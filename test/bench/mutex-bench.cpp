#include "coro/go.h"
#include "sched/thread-pool.h"
#include "sync/mutex.h"
#include "wait-group.h"

#include <gtest/gtest.h>
#include <nanobench.h>

#include <iostream>

class MutexBench : public ::testing::Test {};

using namespace ct;

namespace ct_test {

TEST_F(MutexBench, Basic) {
  std::cout << "Hello!\n";
  ankerl::nanobench::Bench().run("mutex critical section", [&] {
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
  });
}

} // namespace ct_test
