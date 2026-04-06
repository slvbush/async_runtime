#include "coro/go.h"
#include "coro/yield.h"
#include "sched/run-loop.h"
#include "sched/thread-pool.h"
#include "sync/channel.h"
#include "wait-group.h"

#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <random>

namespace ct_test {

using namespace ct;

struct CheckSum {
  std::atomic<std::int64_t> produced{0};
  std::atomic<std::int64_t> consumed{0};

  void produce(std::int64_t v) {
    produced.fetch_add(v, std::memory_order_relaxed);
  }

  void consume(std::int64_t v) {
    consumed.fetch_add(v, std::memory_order_relaxed);
  }

  bool validate() const {
    return produced.load() == consumed.load();
  }
};

TEST(ChannelThreadPoolTest, SingleProducerSingleConsumer) {
  constexpr auto DURATION = std::chrono::seconds{2};
  sched::ThreadPool pool{4};

  sync::BufferedChannel<std::int64_t> ch{4};

  CheckSum checksum;
  WaitGroup wg;
  wg.add(2);

  constexpr std::int64_t END = -1;

  // Producer
  coro::go(pool, [&, start = std::chrono::steady_clock::now()](this auto) -> coro::Coroutine {
    std::mt19937 twister{42}; // NOLINT(cert-msc51-cpp,cert-msc32-c)

    while (std::chrono::steady_clock::now() - start < DURATION) {
      auto v = static_cast<std::int64_t>(twister());
      co_await ch.send(v);
      checksum.produce(v);
    }

    co_await ch.send(END);
    wg.done();
  });

  // Consumer
  coro::go(pool, [&](this auto) -> coro::Coroutine {
    while (true) {
      std::int64_t msg = co_await ch.recv();

      if (msg == END) {
        break;
      }
      checksum.consume(msg);
    }
    wg.done();
  });

  pool.run();
  wg.wait();

  ASSERT_TRUE(checksum.validate());
}

TEST(ChannelThreadPoolTest, YieldStressTest) {
  sched::ThreadPool pool{4};
  sync::BufferedChannel<std::int64_t> ch{1};

  constexpr int COUNT = 1'000'000;
  constexpr std::int64_t END = -1;

  WaitGroup wg;
  wg.add(2);

  coro::go(pool, [&](this auto) -> coro::Coroutine {
    for (int i = 0; i < COUNT; ++i) {
      co_await ch.send(i);
      co_await coro::yield();
    }
    co_await ch.send(END);
    wg.done();
  });

  coro::go(pool, [&](this auto) -> coro::Coroutine {
    while (true) {
      int val = co_await ch.recv();
      if (val == END) {
        break;
      }
      co_await coro::yield();
    }
    wg.done();
  });

  pool.run();
  wg.wait();
}

TEST(ChannelThreadPoolTest, MultipleProducersMultipleConsumers) {
  constexpr std::size_t PRODUCERS = 3;
  constexpr std::size_t CONSUMERS = 3;
  constexpr std::size_t BUFFER = 4;
  constexpr auto DURATION = std::chrono::seconds{2};

  sched::ThreadPool pool{4};
  sync::BufferedChannel<std::int64_t> ch{BUFFER};

  CheckSum checksum;
  WaitGroup wg;
  wg.add(PRODUCERS + CONSUMERS);

  std::atomic_size_t producers_left{PRODUCERS};
  const std::int64_t END = -1;

  for (std::size_t i = 0; i < PRODUCERS; ++i) {
    coro::go(pool, [&, i, ch, start = std::chrono::steady_clock::now()](this auto) -> coro::Coroutine {
      std::mt19937 twister{static_cast<uint32_t>(i)};

      while (std::chrono::steady_clock::now() - start < DURATION) {
        auto v = static_cast<std::int64_t>(twister());
        co_await ch.send(v);
        checksum.produce(v);
      }

      if (producers_left.fetch_sub(1) == 1) {
        for (size_t j = 0; j < CONSUMERS; ++j) {
          co_await ch.send(END);
        }
      }

      wg.done();
    });
  }

  for (std::size_t j = 0; j < CONSUMERS; ++j) {
    coro::go(pool, [&, ch](this auto) -> coro::Coroutine {
      while (true) {
        std::int64_t msg = co_await ch.recv();

        if (msg == END) {
          break;
        }
        checksum.consume(msg);
      }
      wg.done();
    });
  }

  pool.run();
  wg.wait();

  ASSERT_TRUE(checksum.validate());
}

} // namespace ct_test
