#include "list-element.h"

#include <memory>

namespace ct::intrusive {

ListElementImpl::ListElementImpl() {
  loop();
}

void ListElementImpl::unlink() {
  prev->next = next;
  next->prev = prev;
  loop();
}

ListElementImpl::~ListElementImpl() {
  unlink();
}

void link(ListElementImpl* lhs, ListElementImpl* rhs) {
  lhs->next = rhs;
  rhs->prev = lhs;
}

void ListElementImpl::loop() {
  prev = this;
  next = this;
}
} // namespace ct::intrusive
