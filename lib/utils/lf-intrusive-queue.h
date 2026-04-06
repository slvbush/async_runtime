#pragma once

#include <type_traits>

#include "list-element.h"

namespace ct::intrusive {

template <typename T>
  requires(std::is_base_of_v<AtomicSingleLinkedListElementImpl, T>)
class LockFreeIntrusiveQueue {
  std::atomic<T *> head;
  std::atomic<T *> tail;

public:
  LockFreeIntrusiveQueue(T *dummy) : head(dummy), tail(dummy) {}

  T *pop() {
    while (true) {
      T *old_head = head;
      T *old_tail = tail;
      T *next_head = cast_back(cast_to_atomic(head.load())->next);
      if (old_head == old_tail) {
        if (!next_head) {
          return nullptr;
        } else {
          tail.compare_exchange_strong(old_tail, next_head);
        }
      } else {
        if (head.compare_exchange_strong(old_head, next_head)) {
          return next_head;
        }
      }
    }
  }

  void push(T *awaiter) {
    while (true) {
      T *old_tail = tail.load();
      AtomicSingleLinkedListElementImpl *null_tail = nullptr;
      if (cast_to_atomic(tail.load())
              ->next.compare_exchange_strong(null_tail, awaiter)) {
        tail.compare_exchange_strong(old_tail, awaiter);
        return;
      } else {
        tail.compare_exchange_strong(
            old_tail, cast_back(cast_to_atomic(tail.load())->next.load()));
      }
    }
  }

  T *get_dummy() { return head.load(std::memory_order_relaxed); }

private:
  // friend class ct::sync::Mutex;

  AtomicSingleLinkedListElementImpl *cast_to_atomic(T *node_ptr) {
    return static_cast<AtomicSingleLinkedListElementImpl *>(node_ptr);
  }

  T *cast_back(AtomicSingleLinkedListElementImpl *node_ptr) {
    return static_cast<T *>(node_ptr);
  }
};
} // namespace ct::intrusive