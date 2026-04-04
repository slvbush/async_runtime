#pragma once

#include <condition_variable>
#include <cstddef>
#include <mutex>

namespace ct_test {

// Naive impl for tests
class WaitGroup {
public:
  void add(std::size_t count);

  void done();

  void wait();

private:
  std::mutex _lock;
  std::condition_variable _zero_work_cv;
  std::size_t _work{0};
};

} // namespace ct_test
