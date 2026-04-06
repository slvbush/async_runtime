#include "list-element.h"

#include <memory>

namespace ct::intrusive {

ListElementImpl::ListElementImpl() {
  loop();
}

void ListElementImpl::unlink() {
  _prev->_next = _next;
  _next->_prev = _prev;
  loop();
}

ListElementImpl::~ListElementImpl() {
  unlink();
}

void link(ListElementImpl* lhs, ListElementImpl* rhs) {
  lhs->_next = rhs;
  rhs->_prev = lhs;
}

void ListElementImpl::loop() {
  _prev = this;
  _next = this;
}
} // namespace ct::intrusive
