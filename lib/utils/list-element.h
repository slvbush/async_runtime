#pragma once

#include <atomic>

namespace ct::intrusive {

class DefaultTag;

class AtomicSingleLinkedListElementImpl {
public: // temporary
  std::atomic<AtomicSingleLinkedListElementImpl*> _next{nullptr};
};

class SingleLinkedListElementImpl {
public:
  SingleLinkedListElementImpl* _next{nullptr};
};

class ListElementImpl {
  ListElementImpl();

  ~ListElementImpl();

  void loop();
  void unlink();

  friend void link(ListElementImpl* lhs, ListElementImpl* rhs);

  template <typename T, typename Tag>
  friend class List;

  template <typename Tag>
  friend class ListElement;

  ListElementImpl* _prev;
  ListElementImpl* _next;
};

template <typename = DefaultTag>
class ListElement : ListElementImpl {
  template <typename T, typename ListTag>
  friend class List;
};
} // namespace ct::intrusive
