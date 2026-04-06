#include "sched/thread-pool.h"

#include "alloc-count.h"
#include "lambda-resumable.h"
#include "sched/resumable.h"
#include "wait-group.h"

#include <gtest/gtest.h>

#include <atomic>
#include <thread>

namespace ct_test {

using namespace std::chrono_literals;

using namespace ct::sched;

class ThreadPoolTest : public ::testing::Test {
protected:
  template <typename F>
  static void go(ThreadPool& pool, F&& fn) {
    auto* t = new LambdaResumable(std::forward<F>(fn));
    pool.spawn(*t);
  }
};

TEST_F(ThreadPoolTest, WaitTask) {
  ThreadPool pool{4};
  pool.run();

  WaitGroup wg;
  wg.add(1);

  go(pool, [&wg] {
    wg.done();
  });

  wg.wait();
}

TEST_F(ThreadPoolTest, Wait) {
  ThreadPool pool{4};
  pool.run();

  WaitGroup wg;
  wg.add(1);

  go(pool, [&wg] {
    std::this_thread::sleep_for(300ms);
    wg.done();
  });

  wg.wait();
}

TEST_F(ThreadPoolTest, MultiWait) {
  ThreadPool pool{1};
  pool.run();

  for (std::size_t i = 0; i < 3; ++i) {
    WaitGroup wg;
    wg.add(1);

    go(pool, [&wg] {
      std::this_thread::sleep_for(300ms);
      wg.done();
    });

    wg.wait();
  }
}

TEST_F(ThreadPoolTest, ManyTasks) {
  ThreadPool pool{4};
  pool.run();

  static const std::size_t TASKS = 17;
  WaitGroup wg;

  for (std::size_t i = 0; i < TASKS; ++i) {
    wg.add(1);
    go(pool, [&wg] {
      wg.done();
    });
  }

  wg.wait();
}

TEST_F(ThreadPoolTest, TwoPools) {
  ThreadPool pool1{1};
  ThreadPool pool2{1};

  pool1.run();
  pool2.run();

  auto start = std::chrono::steady_clock::now();

  WaitGroup wg1;
  wg1.add(1);
  go(pool1, [&wg1] {
    std::this_thread::sleep_for(300ms);
    wg1.done();
  });

  WaitGroup wg2;
  wg2.add(1);
  go(pool2, [&wg2] {
    std::this_thread::sleep_for(300ms);
    wg2.done();
  });

  wg2.wait();
  wg1.wait();

  auto elapsed = std::chrono::steady_clock::now() - start;
  EXPECT_LT(elapsed, 450ms);
}

TEST_F(ThreadPoolTest, DoNotSpin) {
  ThreadPool pool{4};
  pool.run();

  WaitGroup wg;

  for (std::size_t i = 0; i < 4; ++i) {
    wg.add(1);
    go(pool, [&wg] {
      std::this_thread::sleep_for(100ms);
      wg.done();
    });
  }

  auto cpu_start = std::chrono::steady_clock::now();
  std::this_thread::sleep_for(1s);
  wg.wait();

  auto cpu_end = std::chrono::steady_clock::now();

  EXPECT_LT(cpu_end - cpu_start, 2s);
}

TEST_F(ThreadPoolTest, GoAfterWait) {
  ThreadPool pool{4};
  pool.run();

  WaitGroup wg;
  wg.add(1);

  go(pool, [&] {
    std::this_thread::sleep_for(500ms);
    wg.add(1);

    go(pool, [&] {
      std::this_thread::sleep_for(500ms);
      wg.done();
    });

    wg.done();
  });

  wg.wait();
}

TEST_F(ThreadPoolTest, CrossGo) {
  ThreadPool pool1{1};
  ThreadPool pool2{1};

  pool1.run();
  pool2.run();

  WaitGroup wg;
  wg.add(1);

  go(pool1, [&] {
    go(pool2, [&] {
      wg.done();
    });
  });

  wg.wait();
}

TEST_F(ThreadPoolTest, ManyThreads) {
  ThreadPool pool{3};
  pool.run();

  WaitGroup wg;

  for (std::size_t i = 0; i < 4; ++i) {
    wg.add(1);
    go(pool, [&wg] {
      std::this_thread::sleep_for(750ms);
      wg.done();
    });
  }

  auto start = std::chrono::steady_clock::now();
  wg.wait();

  auto elapsed = std::chrono::steady_clock::now() - start;

  EXPECT_GT(elapsed, 1s);
}

TEST_F(ThreadPoolTest, Lifetime) {
  ThreadPool pool{4};
  pool.run();

  struct Widget {};

  auto w = std::make_shared<Widget>();

  WaitGroup wg;
  for (int i = 0; i < 4; ++i) {
    wg.add(1);
    go(pool, [w, &wg] {
      wg.done();
    });
  }

  std::this_thread::sleep_for(500ms);
  EXPECT_EQ(w.use_count(), 1);

  wg.wait();
}

TEST_F(ThreadPoolTest, Counter) {
  ThreadPool pool{4};
  pool.run();

  std::atomic<std::size_t> counter{0};
  static const std::size_t TASKS = 100'500;

  WaitGroup wg;
  for (std::size_t i = 0; i < TASKS; ++i) {
    wg.add(1);
    go(pool, [&] {
      counter.fetch_add(1, std::memory_order::seq_cst);
      wg.done();
    });
  }

  wg.wait();

  EXPECT_EQ(counter.load(), TASKS);
}

TEST_F(ThreadPoolTest, NoAllocations) {
  const std::size_t RESUMABLES = 100;

  std::size_t count = 0;

  struct StackResumable : Resumable<IntrusiveListScheduler> {
    void resume(IntrusiveListScheduler& /*unused*/) noexcept final {
      resumed = true;
    }

    bool resumed = false;
  };

  std::array<StackResumable, RESUMABLES> resumables;

  {
    ThreadPool pool{4};
    pool.run();

    count = g_allocation_count.load();

    for (auto& resumable : resumables) {
      pool.spawn(resumable);
    }
  }

  for (const auto& resumable : resumables) {
    EXPECT_TRUE(resumable.resumed);
  }

  EXPECT_EQ(g_allocation_count.load(), count);
}

} // namespace ct_test
