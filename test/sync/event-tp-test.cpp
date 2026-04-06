
#include "coro/go.h"
#include "sched/thread-pool.h"
#include "sync/event.h"
#include "wait-group.h"

#include <gtest/gtest.h>

#include <cstddef>

namespace ct_test {

using namespace ct;

TEST(EventThreadPoolTest, ConsumerProducer) {
  sched::ThreadPool pool{4};
  pool.run();

  size_t iterations = 0;
  const size_t LIMIT = 10000;
  const size_t STEP = 7;

  while (iterations++ < LIMIT) {
    WaitGroup wg;
    sync::Event event;
    int data = 0;

    for (std::size_t i = 0; i < iterations % STEP; ++i) {
      wg.add(1);
      coro::go(pool, [&](this auto) -> coro::Coroutine {
        co_await event.wait();
        EXPECT_EQ(data, 1);
        wg.done();
      });
    }

    wg.add(1);
    coro::go(pool, [&](this auto) -> coro::Coroutine {
      data = 1;
      event.emit();
      wg.done();
      co_return;
    });

    wg.wait();
  }
}

} // namespace ct_test
