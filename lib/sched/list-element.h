#pragma once

namespace ct::intrusive {

class DefaultTag;

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

  ListElementImpl* prev;
  ListElementImpl* next;
};

template <typename = DefaultTag>
class ListElement : ListElementImpl {
  template <typename T, typename ListTag>
  friend class List;
};
} // namespace ct::intrusive
