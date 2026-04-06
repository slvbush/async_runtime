#pragma once

#include "list-element.h"

#include <type_traits>

namespace ct::intrusive::lockfree {

// Simplified implementation of Treiber stack
template <typename T>
  requires (std::is_base_of_v<SingleLinkedListElementImpl, T>)
class LockFreeIntrusiveStack {
  std::atomic<T*> _head;

public:
  void push(T* aw) {
    T* expected = _head.load();

    do {
      cast_to_non_atomic(aw)->_next = expected;
    } while (!_head.compare_exchange_weak(expected, aw));
  }

  LockFreeIntrusiveStack(T* dummy)
      : _head(dummy) {}

  ~LockFreeIntrusiveStack() {
    clear();
  }

private:
  friend class Mutex;

  void clear() {
    T* node = _head.exchange(nullptr);
    while (node) {
      T* to_delete = static_cast<T*>(node);
      node = static_cast<T*>(cast_to_non_atomic(node)->_next);
      delete to_delete;
    }
  }

  SingleLinkedListElementImpl* cast_to_non_atomic(T* node_ptr) {
    return static_cast<SingleLinkedListElementImpl*>(node_ptr);
  }
};
} // namespace ct::intrusive::lockfree
