#pragma once

#include "../coro/coroutine.h"

#include <queue>

#include <coroutine>
#include <cstddef>
#include <memory>
#include <mutex>
#include <optional>

namespace ct::sync {

// Buffered MPMC Channel
// https://tour.golang.org/concurrency/3

template <typename T> class BufferedChannel {
public:
  explicit BufferedChannel(std::size_t capacity)
      : impl(std::make_shared<State>(capacity)) {}

private:
  struct State;

  template <typename Awaiter>
  static void enqueue_awaiter(Awaiter &awaiter, Awaiter *&head,
                              Awaiter *&tail) {
    if (!(head || tail)) {
      head = &awaiter;
      tail = &awaiter;
    } else if (head && tail) {
      tail->next = &awaiter;
      tail = &awaiter;
    }
  }

  struct Send {};

  struct Recv {};

  static void move_value_to_buffer(auto &state, T &val) {
    state.buffer.push(std::move(val));
  }

  static void move_value_to_value(T &lhs, T &rhs) {
    std::construct_at(&rhs, std::move(lhs));
  }

  static void move_from_buffer_to_value(auto &state, T &val) {
    std::construct_at(&val, std::move(state.buffer.front()));
    state.buffer.pop();
  }

  template <typename ActionT, typename FirstAwaiterT, typename SecondAwaiterT>
  static bool try_sync(auto &state, FirstAwaiterT &that, SecondAwaiterT *&head,
                       SecondAwaiterT *&tail, bool capacity_valid) {
    if (capacity_valid || state.capacity == 0) {
      if (head == tail && !head) {
        if (state.capacity > 0) {
          if constexpr (std::is_same_v<ActionT, Send>) {
            move_value_to_buffer(state, that.val);
          } else {
            move_from_buffer_to_value(state, that.val);
          }
          return true;
        }
      } else {
        SecondAwaiterT *aw = head;
        if (head == tail) {
          tail = nullptr;
        }
        head = head->next;
        if constexpr (std::is_same_v<ActionT, Send>) {
          move_value_to_value(that.val, aw->val);
        } else {
          if (state.capacity > 0) {
            move_from_buffer_to_value(state, that.val);
            move_value_to_buffer(state, aw->val);
          } else {
            move_value_to_value(aw->val, that.val);
          }
        }
        coro::Coroutine::current_scheduler()->spawn(
            aw->awaiting_coroutine.promise());
        return true;
      }
    }
    return false;
  }

  class SendAwaiter {
  public:
    template <typename U = T>
    SendAwaiter(BufferedChannel &ch, U &&val)
        : state_ptr(ch.impl), val(std::forward<U>(val)) {}

    bool await_ready() {
      auto &state = *state_ptr;
      std::scoped_lock lock(state.buffer_mutex, state.receivers_mutex);
      return try_sync<Send>(state, *this, state.recv_head, state.recv_tail,
                            state.buffer.size() < state.capacity);
    }

    bool
    await_suspend(std::coroutine_handle<coro::Coroutine::promise_type> handle) {
      auto &state = *state_ptr;
      std::scoped_lock lock(state.buffer_mutex, state.receivers_mutex,
                            state.senders_mutex);
      if (try_sync<Send>(state, *this, state.recv_head, state.recv_tail,
                         state.buffer.size() < state.capacity)) {
        return false;
      }
      awaiting_coroutine = handle;
      enqueue_awaiter(*this, state.send_head, state.send_tail);
      return true;
    }

    void await_resume() {}

    std::coroutine_handle<coro::Coroutine::promise_type> awaiting_coroutine;
    SendAwaiter *next{nullptr};
    std::shared_ptr<State> state_ptr;

    union {
      T val;
    };
  };

  class RecvAwaiter {
  public:
    bool await_ready() {
      auto &state = *state_ptr;
      std::scoped_lock lock(state.buffer_mutex, state.senders_mutex);
      return try_sync<Recv>(state, *this, state.send_head, state.send_tail,
                            state.buffer.size() > 0);
    }

    bool
    await_suspend(std::coroutine_handle<coro::Coroutine::promise_type> handle) {
      auto &state = *state_ptr;
      std::scoped_lock lock(state.buffer_mutex, state.senders_mutex,
                            state.receivers_mutex);
      if (try_sync<Recv>(state, *this, state.send_head, state.send_tail,
                         state.buffer.size() > 0)) {
        return false;
      }
      awaiting_coroutine = handle;
      enqueue_awaiter(*this, state.recv_head, state.recv_tail);
      return true;
    }

    T await_resume() { return std::move(val); }

  private:
    friend class BufferedChannel;

    RecvAwaiter(BufferedChannel &ch) : state_ptr(ch.impl) {}

    std::coroutine_handle<coro::Coroutine::promise_type> awaiting_coroutine;
    RecvAwaiter *next{nullptr};
    std::shared_ptr<State> state_ptr;

    union {
      T val;
    };
  };

public:
  SendAwaiter send(T &&value) { return {*this, std::move(value)}; }

  SendAwaiter send(const T &value) { return {*this, value}; }

  RecvAwaiter recv() { return {*this}; }

private:
  struct State {
    explicit State(std::size_t capacity) : capacity(capacity) {}

    std::size_t capacity;
    SendAwaiter *send_head{nullptr};
    SendAwaiter *send_tail{nullptr};
    RecvAwaiter *recv_head{nullptr};
    RecvAwaiter *recv_tail{nullptr};
    std::queue<T> buffer;
    std::mutex senders_mutex;
    std::mutex receivers_mutex;
    std::mutex buffer_mutex;
  };

  std::shared_ptr<State> impl;
};

} // namespace ct::sync
