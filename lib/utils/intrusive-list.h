#pragma once

#include "list-element.h"

#include <iterator>
#include <type_traits>

namespace ct::intrusive {

class DefaultTag;

// Cut-off interface intrusive list
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
      ref_node = as_tagged_ptr(ref_node->_next);
      return *this;
    }

    IntrusiveListIterator operator++(int) noexcept {
      IntrusiveListIterator ans(*this);
      ++(*this);
      return ans;
    }

    IntrusiveListIterator& operator--() noexcept {
      ref_node = as_tagged_ptr(ref_node->_prev);
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

  List() noexcept = default;

  ~List() = default;

  List(const List&) = delete;
  List& operator=(const List&) = delete;

  List(List&& other) noexcept = default;

  List& operator=(List&& other) noexcept = default;

  bool empty() const noexcept {
    return begin() == end();
  }

  size_t size() const noexcept {
    return std::distance(begin(), end());
  }

  T& front() noexcept {
    return *begin();
  }

  void push_front(T& value) noexcept {
    insert(begin(), value);
  }

  void push_back(T& value) noexcept {
    insert(end(), value);
  }

  void pop_front() noexcept {
    erase(begin());
  }

  Iterator begin() noexcept {
    return Iterator{_sentinel._next};
  }

  ConstIterator begin() const noexcept {
    return ConstIterator{_sentinel._next};
  }

  Iterator end() noexcept {
    return Iterator{std::addressof(_sentinel)};
  }

  ConstIterator end() const noexcept {
    return ConstIterator{std::addressof(_sentinel)};
  }

  Iterator insert(ConstIterator pos, T& element) noexcept {
    if (std::addressof(as_node(element)) == pos.ref_node) {
      return Iterator{pos.ref_node};
    }
    as_node(element).unlink();
    link(pos.ref_node->_prev, std::addressof(as_node(element)));
    link(std::addressof(as_node(element)), pos.ref_node);
    return Iterator{std::addressof(as_node(element))};
  }

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
