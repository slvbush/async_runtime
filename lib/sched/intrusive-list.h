#pragma once

#include "list-element.h"

#include <iterator>
#include <type_traits>

namespace ct::intrusive { // this code was taken from intrusive prac, interface is a little smaller, i cut off
                          // unnecessary stuff

class DefaultTag;

template <typename T, typename Tag = DefaultTag>
class List {
  static_assert(std::is_base_of_v<ListElement<Tag>, T>, "T must derive from ListElement");

private:
  template <bool isConst>
  class IntrusiveListIterator {
  public:
    using iterator_category = std::bidirectional_iterator_tag;
    using value_type = T;
    using pointer = std::conditional_t<isConst, const T*, T*>;
    using difference_type = std::ptrdiff_t;
    using reference = std::conditional_t<isConst, const T&, T&>;

    IntrusiveListIterator() = default;

    operator IntrusiveListIterator<true>() const noexcept {
      return IntrusiveListIterator<true>(ref_node);
    }

    reference operator*() const noexcept {
      return *as_t_ptr(ref_node);
    }

    pointer operator->() const noexcept {
      return as_t_ptr(ref_node);
    }

    IntrusiveListIterator& operator++() noexcept {
      ref_node = as_tagged_ptr(ref_node->next);
      return *this;
    }

    IntrusiveListIterator operator++(int) noexcept {
      IntrusiveListIterator ans(*this);
      ++(*this);
      return ans;
    }

    IntrusiveListIterator& operator--() noexcept {
      ref_node = as_tagged_ptr(ref_node->prev);
      return *this;
    }

    IntrusiveListIterator operator--(int) noexcept {
      IntrusiveListIterator ans(*this);
      --(*this);
      return ans;
    }

    friend bool operator==(const IntrusiveListIterator& lhs, const IntrusiveListIterator& rhs) noexcept = default;

    friend bool operator!=(const IntrusiveListIterator& lhs, const IntrusiveListIterator& rhs) noexcept = default;

  private:
    friend class List;

    static ListElement<Tag>* as_tagged_ptr(ListElementImpl* node) {
      return static_cast<ListElement<Tag>*>(node);
    }

    static T* as_t_ptr(ListElement<Tag>* node) {
      return static_cast<T*>(node);
    }

    explicit IntrusiveListIterator(ListElementImpl* node) noexcept
        : ref_node(as_tagged_ptr(node)) {}

    ListElement<Tag>* ref_node;
  };

public:
  using Iterator = IntrusiveListIterator<false>;
  using ConstIterator = IntrusiveListIterator<true>;

  // O(1)
  List() noexcept = default;

  // O(1)
  ~List() = default;

  List(const List&) = delete;
  List& operator=(const List&) = delete;

  // O(1)
  List(List&& other) noexcept = default;

  // O(1)
  List& operator=(List&& other) noexcept = default;

  // O(1)
  bool empty() const noexcept {
    return begin() == end();
  }

  // O(n)
  size_t size() const noexcept {
    return std::distance(begin(), end());
  }

  // O(1)
  T& front() noexcept {
    return *begin();
  }

  // O(1)
  void push_front(T& value) noexcept {
    insert(begin(), value);
  }

  // O(1)
  void push_back(T& value) noexcept {
    insert(end(), value);
  }

  // O(1)
  void pop_front() noexcept {
    erase(begin());
  }

  // O(1)
  Iterator begin() noexcept {
    return Iterator{_sentinel.next};
  }

  // O(1)
  ConstIterator begin() const noexcept {
    return ConstIterator{_sentinel.next};
  }

  // O(1)
  Iterator end() noexcept {
    return Iterator{std::addressof(_sentinel)};
  }

  // O(1)
  ConstIterator end() const noexcept {
    return ConstIterator{std::addressof(_sentinel)};
  }

  // O(1)
  Iterator insert(ConstIterator pos, T& element) noexcept {
    if (std::addressof(as_node(element)) == pos.ref_node) {
      return Iterator{pos.ref_node};
    }
    as_node(element).unlink();
    link(pos.ref_node->prev, std::addressof(as_node(element)));
    link(std::addressof(as_node(element)), pos.ref_node);
    return Iterator{std::addressof(as_node(element))};
  }

  // O(1)
  Iterator erase(ConstIterator pos) noexcept {
    Iterator ans(std::next(pos).ref_node);
    pos.ref_node->unlink();
    return ans;
  }

private:
  using node_t = ListElement<Tag>;

  static node_t& as_node(T& element) {
    return static_cast<node_t&>(element);
  }

  mutable node_t _sentinel;
};

} // namespace ct::intrusive
