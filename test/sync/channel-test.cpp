#include "sync/channel.h"

#include "coro/go.h"
#include "coro/yield.h"
#include "sched/run-loop.h"

#include <gtest/gtest.h>

namespace ct_test {

using namespace ct;

TEST(ChannelTest, BasicSendRecv) {
  sched::RunLoop loop;

  bool done = false;

  coro::go(loop, [&](this auto) -> coro::Coroutine {
    sync::BufferedChannel<int> ch{4};

    co_await ch.send(10);
    co_await ch.send(20);
    co_await ch.send(30);

    EXPECT_EQ(co_await ch.recv(), 10);
    EXPECT_EQ(co_await ch.recv(), 20);
    EXPECT_EQ(co_await ch.recv(), 30);

    done = true;
  });

  size_t tasks = loop.run();

  ASSERT_EQ(tasks, 1);
  ASSERT_TRUE(done);
}

TEST(ChannelTest, MoveOnlyItem) {
  struct MoveOnly {
    MoveOnly() = default;

    MoveOnly(const MoveOnly&) = delete;
    MoveOnly& operator=(const MoveOnly&) = delete;

    MoveOnly(MoveOnly&&) = default;
    MoveOnly& operator=(MoveOnly&&) = default;
  };

  sched::RunLoop loop;

  coro::go(loop, [&](this auto) -> coro::Coroutine {
    sync::BufferedChannel<MoveOnly> ch{3};

    co_await ch.send({});
    co_await ch.send({});

    co_await ch.recv();
    co_await ch.recv();
  });

  loop.run();
}

TEST(ChannelTest, NonDefaultConstructibleItem) {
  struct NonDefaultConstructible {
    explicit NonDefaultConstructible(int /*unused*/) {}
  };

  sched::RunLoop loop;

  coro::go(loop, [&](this auto) -> coro::Coroutine {
    sync::BufferedChannel<NonDefaultConstructible> ch{1};

    co_await ch.send(NonDefaultConstructible{42});
    co_await ch.recv();
  });

  loop.run();
}

TEST(ChannelTest, SuspendReceiver) {
  sched::RunLoop loop;

  sync::BufferedChannel<int> ch{2};

  bool got = false;

  coro::go(loop, [&](this auto) -> coro::Coroutine {
    int v = co_await ch.recv();
    EXPECT_EQ(v, 99);
    got = true;
  });

  {
    size_t tasks = loop.run();
    ASSERT_LT(tasks, 3);
    ASSERT_FALSE(got);
  }

  coro::go(loop, [&](this auto) -> coro::Coroutine {
    co_await ch.send(99);
  });

  loop.run();

  ASSERT_TRUE(got);
}

TEST(ChannelTest, SuspendSender) {
  sched::RunLoop loop;

  sync::BufferedChannel<int> ch{2};

  int sent = 0;

  coro::go(loop, [&](this auto) -> coro::Coroutine {
    for (int v = 0; v < 3; ++v) {
      co_await ch.send(v);
      ++sent;
    }
  });

  {
    size_t tasks = loop.run();
    ASSERT_LT(tasks, 6);
    ASSERT_EQ(sent, 2);
  }

  bool done = false;

  coro::go(loop, [&](this auto) -> coro::Coroutine {
    {
      int v = co_await ch.recv(); // wakes sender
      EXPECT_EQ(v, 0);
    }

    co_await coro::yield();

    EXPECT_EQ(co_await ch.recv(), 1);
    EXPECT_EQ(co_await ch.recv(), 2);

    done = true;
  });

  loop.run_at_most(2);
  ASSERT_EQ(sent, 3);

  loop.run();
  ASSERT_TRUE(done);
}

TEST(ChannelTest, FIFO) {
  sched::RunLoop loop;

  sync::BufferedChannel<int> ch{8};

  constexpr int N = 128;

  coro::go(loop, [&](this auto) -> coro::Coroutine {
    for (int i = 0; i < N; ++i) {
      co_await ch.send(i);
      if (i % 3 == 0) {
        co_await coro::yield();
      }
    }
  });

  bool done = false;

  coro::go(loop, [&](this auto) -> coro::Coroutine {
    for (int i = 0; i < N; ++i) {
      int v = co_await ch.recv();
      EXPECT_EQ(v, i);
      if (i % 2 == 0) {
        co_await coro::yield();
      }
    }
    done = true;
  });

  {
    size_t tasks = loop.run();
    ASSERT_LT(tasks, 512);
  }

  ASSERT_TRUE(done);
}

TEST(ChannelTest, ZeroBufferRendezvous) {
  sched::RunLoop loop;

  // Zero-capacity (rendezvous) channel
  sync::BufferedChannel<int> ch{0};

  int received = -1;
  bool done = false;

  coro::go(loop, [&](this auto) -> coro::Coroutine {
    received = co_await ch.recv();
    done = true;
  });

  {
    size_t tasks = loop.run();
    ASSERT_LT(tasks, 3);
    ASSERT_FALSE(done);
  }

  coro::go(loop, [&](this auto) -> coro::Coroutine {
    co_await ch.send(42);
  });

  loop.run();

  ASSERT_TRUE(done);
  ASSERT_EQ(received, 42);
}

TEST(ChannelTest, ZeroBufferRendezvousReverse) {
  sched::RunLoop loop;

  // Zero-capacity (rendezvous) channel
  sync::BufferedChannel<int> ch{0};

  int received = -1;
  bool done = false;

  coro::go(loop, [&](this auto) -> coro::Coroutine {
    co_await ch.send(42);
  });

  {
    size_t tasks = loop.run();
    ASSERT_LT(tasks, 3);
    ASSERT_FALSE(done);
  }

  coro::go(loop, [&](this auto) -> coro::Coroutine {
    received = co_await ch.recv();
    done = true;
  });

  loop.run();

  ASSERT_TRUE(done);
  ASSERT_EQ(received, 42);
}

namespace {

struct Counter {
  inline static int moves = 0;
  inline static int copies = 0;

  int value = 0;

  Counter() = default;

  explicit Counter(int v)
      : value(v) {}

  Counter(const Counter& other)
      : value(other.value) {
    ++copies;
  }

  Counter& operator=(const Counter& other) = delete;

  Counter(Counter&& other) noexcept
      : value(other.value) {
    ++moves;
  }

  Counter& operator=(Counter&& other) noexcept = delete;

  static void reset() {
    moves = 0;
    copies = 0;
  }
};

} // namespace

TEST(ChannelTest, ConsumerProducerExchange) {
  sched::RunLoop loop;

  Counter::reset();

  sync::BufferedChannel<Counter> ch{1};

  int result = 0;
  bool done = false;

  // Consumer
  coro::go(loop, [&](this auto) -> coro::Coroutine {
    Counter out = co_await ch.recv();
    result = out.value;
    done = true;
  });

  // Producer
  coro::go(loop, [&](this auto) -> coro::Coroutine {
    co_await ch.send(Counter{123});
  });

  loop.run();

  ASSERT_TRUE(done);
  ASSERT_EQ(result, 123);

  ASSERT_EQ(Counter::copies, 0);
  ASSERT_LE(Counter::moves, 3);
}

TEST(ChannelTest, ProducerConsumerExchange) {
  sched::RunLoop loop;

  Counter::reset();

  sync::BufferedChannel<Counter> ch{0};

  int result = 0;
  bool done = false;

  // Producer
  coro::go(loop, [&](this auto) -> coro::Coroutine {
    co_await ch.send(Counter{123});
  });

  // Consumer
  coro::go(loop, [&](this auto) -> coro::Coroutine {
    Counter out = co_await ch.recv();
    result = out.value;
    done = true;
  });

  loop.run();

  ASSERT_TRUE(done);
  ASSERT_EQ(result, 123);

  ASSERT_EQ(Counter::copies, 0);
  ASSERT_LE(Counter::moves, 3);
}

TEST(ChannelTest, ZeroBufferMultipleProducers) {
  sched::RunLoop loop;

  sync::BufferedChannel<int> ch{0};

  bool consumer_done = false;
  std::vector<int> received;

  coro::go(loop, [&](this auto) -> coro::Coroutine {
    for (int i = 0; i < 3; ++i) {
      int value = co_await ch.recv();
      received.push_back(value);
    }
    consumer_done = true;
  });

  for (int i = 0; i < 3; ++i) {
    coro::go(loop, [&ch, i](this auto) -> coro::Coroutine {
      co_await ch.send(i * 100);
    });
  }

  loop.run();

  EXPECT_TRUE(consumer_done);
  ASSERT_EQ(received.size(), 3);
  EXPECT_EQ(received[0], 0);
  EXPECT_EQ(received[1], 100);
  EXPECT_EQ(received[2], 200);
}

TEST(ChannelTest, ZeroBufferMultipleConsumers) {
  sched::RunLoop loop;

  sync::BufferedChannel<int> ch{0};

  int consumers_done = 0;
  std::vector<int> received;

  for (int i = 0; i < 3; ++i) {
    coro::go(loop, [&](this auto) -> coro::Coroutine {
      int value = co_await ch.recv();
      received.push_back(value);
      consumers_done++;
    });
  }

  coro::go(loop, [&](this auto) -> coro::Coroutine {
    for (int i = 0; i < 3; ++i) {
      co_await ch.send(i * 100);
    }
  });

  loop.run();

  EXPECT_EQ(consumers_done, 3);
  ASSERT_EQ(received.size(), 3);
  EXPECT_EQ(received[0], 0);
  EXPECT_EQ(received[1], 100);
  EXPECT_EQ(received[2], 200);
}

} // namespace ct_test
