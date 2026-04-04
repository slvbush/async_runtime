#pragma once

#include <atomic>
#include <cstddef>

extern std::atomic<std::size_t> g_allocation_count; // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)
