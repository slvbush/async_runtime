#pragma once

#include <type_traits>

#include "list-element.h"

namespace ct::intrusive {

template <typename T>
  requires(std::is_base_of_v<SingleLinkedListElementImpl, T>)
class LockFreeIntrusiveStack {
  std::atomic<T *> head;

public:
  void push(T *aw) {
    T *expected = head.load();

    do {
      cast_to_non_atomic(aw)->next = expected;
    } while (!head.compare_exchange_weak(expected, aw));
  }

  LockFreeIntrusiveStack(T *dummy) : head(dummy) {}

  ~LockFreeIntrusiveStack() { clear(); }

private:
  friend class Mutex;

  void clear() {
    T *node = head.exchange(nullptr);
    while (node) {
      T *to_delete = static_cast<T *>(node);
      node = static_cast<T *>(cast_to_non_atomic(node)->next);
      delete to_delete;
    }
  }

  SingleLinkedListElementImpl *cast_to_non_atomic(T *node_ptr) {
    return static_cast<SingleLinkedListElementImpl *>(node_ptr);
  }
};
} // namespace ct::intrusive