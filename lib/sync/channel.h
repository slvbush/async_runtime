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
template <typename T>
class BufferedChannel {
public:
  explicit BufferedChannel(std::size_t capacity)
      : _channel_state(std::make_shared<State>(capacity)) {}

private:
  struct State;

  template <typename Awaiter>
  static void enqueue_awaiter(Awaiter& awaiter, Awaiter*& head, Awaiter*& tail) {
    if (!(head || tail)) {
      head = &awaiter;
      tail = &awaiter;
    } else if (head && tail) {
      tail->next = &awaiter;
      tail = &awaiter;
    }
  }

  enum class Action {
    Send,
    Recv
  };

  static void move_value_to_buffer(State& state, T&& _val) {
    state._buffer.push(std::move(_val));
  }

  static void move_value_to_value(T&& lhs, T& rhs) {
    std::construct_at(&rhs, std::move(lhs));
  }

  static void move_from_buffer_to_value(State& state, T&& _val) {
    std::construct_at(&_val, std::move(state._buffer.front()));
    state._buffer.pop();
  }

  template <Action action, typename FirstAwaiterT, typename SecondAwaiterT>
  static bool
  try_sync(State& state, FirstAwaiterT& that, SecondAwaiterT*& head, SecondAwaiterT*& tail, bool capacity_valid) {
    if (capacity_valid || state._capacity == 0) {
      if (!head) {
        if (state._capacity > 0) {
          if constexpr (action == Action::Send) {
            move_value_to_buffer(state, std::move(that._val));
          } else {
            move_from_buffer_to_value(state, std::move(that._val));
          }
          return true;
        }
      } else {
        SecondAwaiterT* aw = head;
        if (head == tail) {
          tail = nullptr;
        }
        head = head->next;
        if constexpr (action == Action::Send) {
          move_value_to_value(std::move(that._val), aw->_val);
        } else {
          if (state._capacity > 0) {
            move_from_buffer_to_value(state, std::move(that._val));
            move_value_to_buffer(state, std::move(aw->_val));
          } else {
            move_value_to_value(std::move(aw->_val), that._val);
          }
        }
        coro::Coroutine::current_scheduler()->spawn(aw->awaiting_coroutine.promise());
        return true;
      }
    }
    return false;
  }

  class SendAwaiter {
  public:
    template <typename U>
      requires (std::is_same_v<std::remove_cvref_t<U>, T>)
    SendAwaiter(BufferedChannel& ch, U&& _val)
        : state_ptr(ch._channel_state)
        , _val(std::forward<U>(_val)) {}

    bool await_ready() {
      State& state = *state_ptr;
      std::scoped_lock lock(state._buffer_mutex, state._receivers_mutex);
      return try_sync<Action::Send>(
          state,
          *this,
          state._recv_head,
          state._recv_tail,
          state._buffer.size() < state._capacity
      );
    }

    bool await_suspend(std::coroutine_handle<coro::Coroutine::promise_type> handle) {
      State& state = *state_ptr;
      std::scoped_lock lock(state._buffer_mutex, state._receivers_mutex, state._senders_mutex);
      if (try_sync<
              Action::Send>(state, *this, state._recv_head, state._recv_tail, state._buffer.size() < state._capacity)) {
        return false;
      }
      awaiting_coroutine = handle;
      enqueue_awaiter(*this, state._send_head, state._send_tail);
      return true;
    }

    void await_resume() {}

    std::coroutine_handle<coro::Coroutine::promise_type> awaiting_coroutine;
    SendAwaiter* next{nullptr};
    std::shared_ptr<State> state_ptr;

    T _val;
  };

  class RecvAwaiter {
  public:
    bool await_ready() {
      State& state = *state_ptr;
      std::scoped_lock lock(state._buffer_mutex, state._senders_mutex);
      return try_sync<Action::Recv>(state, *this, state._send_head, state._send_tail, state._buffer.size() > 0);
    }

    bool await_suspend(std::coroutine_handle<coro::Coroutine::promise_type> handle) {
      State& state = *state_ptr;
      std::scoped_lock lock(state._buffer_mutex, state._senders_mutex, state._receivers_mutex);
      if (try_sync<Action::Recv>(state, *this, state._send_head, state._send_tail, state._buffer.size() > 0)) {
        return false;
      }
      awaiting_coroutine = handle;
      enqueue_awaiter(*this, state._recv_head, state._recv_tail);
      return true;
    }

    T await_resume() {
      return std::move(_val);
    }

  private:
    friend class BufferedChannel;

    RecvAwaiter(BufferedChannel& ch)
        : state_ptr(ch._channel_state) {}

    std::coroutine_handle<coro::Coroutine::promise_type> awaiting_coroutine;
    RecvAwaiter* next{nullptr};
    std::shared_ptr<State> state_ptr;

    union {
      T _val;
    };
  };

public:
  SendAwaiter send(T&& value) {
    return {*this, std::move(value)};
  }

  SendAwaiter send(const T& value) {
    return {*this, value};
  }

  RecvAwaiter recv() {
    return {*this};
  }

private:
  struct State {
    explicit State(std::size_t capacity)
        : _capacity(capacity) {}

    std::size_t _capacity;
    SendAwaiter* _send_head{nullptr};
    SendAwaiter* _send_tail{nullptr};
    RecvAwaiter* _recv_head{nullptr};
    RecvAwaiter* _recv_tail{nullptr};
    std::queue<T> _buffer;
    std::mutex _senders_mutex;
    std::mutex _receivers_mutex;
    std::mutex _buffer_mutex;
  };

  std::shared_ptr<State> _channel_state;
};

} // namespace ct::sync
