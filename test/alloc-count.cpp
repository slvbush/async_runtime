#include "alloc-count.h"

std::atomic<std::size_t> g_allocation_count; // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)

#if defined(TSAN)
// Tsan defines it's own allocator mocks
#else

// NOLINTBEGIN(cppcoreguidelines-no-malloc)

#include <cstdlib>
#include <new>

void* operator new(std::size_t n) {
  g_allocation_count++;
  auto* ptr = ::malloc(n);
  if (ptr == nullptr) {
    throw std::bad_alloc{};
  }
  return ptr;
}

void operator delete(void* ptr) noexcept {
  ::free(ptr);
}

void operator delete(void* ptr, std::size_t /*unused*/) noexcept {
  ::free(ptr);
}

void* operator new(std::size_t n, std::align_val_t align) {
  g_allocation_count++;
  auto* ptr = ::aligned_alloc(static_cast<std::size_t>(align), n);
  if (ptr == nullptr) {
    throw std::bad_alloc{};
  }
  return ptr;
}

void operator delete(void* ptr, std::size_t /*unused*/, std::align_val_t /*unused*/) noexcept {
  ::free(ptr);
}

void* operator new(std::size_t n, const std::nothrow_t& /*unused*/) noexcept {
  g_allocation_count++;
  return ::malloc(n);
}

void* operator new(std::size_t n, std::align_val_t align, const std::nothrow_t& /*unused*/) noexcept {
  g_allocation_count++;
  return ::aligned_alloc(static_cast<std::size_t>(align), n);
}

void operator delete(void* ptr, std::align_val_t /*align*/) noexcept {
  ::free(ptr);
}

void* operator new[](std::size_t n) {
  return operator new(n);
}

void operator delete[](void* ptr) noexcept {
  ::free(ptr);
}

void operator delete[](void* ptr, std::size_t /*unused*/) noexcept {
  ::free(ptr);
}

void* operator new[](std::size_t n, std::align_val_t align) {
  return operator new(n, align);
}

void operator delete[](void* ptr, std::size_t /*unused*/, std::align_val_t align) noexcept {
  operator delete(ptr, align);
}

void* operator new[](std::size_t n, const std::nothrow_t& tag) noexcept {
  return operator new(n, tag);
}

void* operator new[](std::size_t n, std::align_val_t align, const std::nothrow_t& tag) noexcept {
  return operator new(n, align, tag);
}

void operator delete[](void* ptr, std::align_val_t /*unused*/) noexcept {
  ::free(ptr);
}

// NOLINTEND(cppcoreguidelines-no-malloc)

#endif
