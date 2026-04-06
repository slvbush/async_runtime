#pragma once

#include "list-element.h"

#include <type_traits>

namespace ct::intrusive::lockfree {

// Lock-free MS-queue implementation
template <typename T>
  requires (std::is_base_of_v<AtomicSingleLinkedListElementImpl, T>)
class LockFreeIntrusiveQueue {
  std::atomic<T*> _head;
  std::atomic<T*> _tail;

public:
  LockFreeIntrusiveQueue(T* dummy)
      : _head(dummy)
      , _tail(dummy) {}

  T* pop() {
    while (true) {
      T* old_head = _head;
      T* old_tail = _tail;
      T* next_head = cast_back(cast_to_atomic(_head.load())->_next);
      if (old_head == old_tail) {
        if (!next_head) {
          return nullptr;
        } else {
          _tail.compare_exchange_strong(old_tail, next_head);
        }
      } else {
        if (_head.compare_exchange_strong(old_head, next_head)) {
          return next_head;
        }
      }
    }
  }

  void push(T* el) {
    while (true) {
      T* old_tail = _tail.load();
      AtomicSingleLinkedListElementImpl* null_tail = nullptr;
      if (cast_to_atomic(_tail.load())->_next.compare_exchange_strong(null_tail, el)) {
        _tail.compare_exchange_strong(old_tail, el);
        return;
      } else {
        _tail.compare_exchange_strong(old_tail, cast_back(cast_to_atomic(_tail.load())->_next.load()));
      }
    }
  }

  T* get_dummy() {
    return _head.load(std::memory_order_relaxed);
  }

private:
  AtomicSingleLinkedListElementImpl* cast_to_atomic(T* node_ptr) {
    return static_cast<AtomicSingleLinkedListElementImpl*>(node_ptr);
  }

  T* cast_back(AtomicSingleLinkedListElementImpl* node_ptr) {
    return static_cast<T*>(node_ptr);
  }
};
} // namespace ct::intrusive::lockfree
