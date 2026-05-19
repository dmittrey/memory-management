#include "pool_registry.h"

#include <array>
#include <atomic>
#include <cstdio>
#include <cstdint>
#include <cstdlib>
#include <cstring>

#include <signal.h>
#include <unistd.h>

namespace pool_registry {
namespace {

constexpr std::size_t kMaxPools = 64;

struct Entry {
  const char* name;
  const void* begin;
  const void* end;
  std::atomic<bool> valid{false};
};

std::array<Entry, kMaxPools> g_entries{};
std::atomic<bool> g_handler_installed{false};

void write_cstr(int fd, const char* s) {
  if (s == nullptr) {
    return;
  }
  const std::size_t len = std::strlen(s);
  if (len > 0) {
    (void)::write(fd, s, len);
  }
}

void write_hex_ptr(int fd, const void* ptr) {
  static const char kHex[] = "0123456789abcdef";
  const auto value = reinterpret_cast<uintptr_t>(ptr);
  char buf[2 + sizeof(uintptr_t) * 2 + 1];
  buf[0] = '0';
  buf[1] = 'x';
  std::size_t pos = 2 + sizeof(uintptr_t) * 2;
  buf[pos] = '\0';
  for (std::size_t i = 0; i < sizeof(uintptr_t) * 2; ++i) {
    const unsigned shift = static_cast<unsigned>((sizeof(uintptr_t) * 2 - 1 - i) * 4);
    buf[2 + i] = kHex[(value >> shift) & 0xF];
  }
  (void)::write(fd, buf, pos);
}

const Entry* find_pool_by_address(const void* addr) {
  const auto p = reinterpret_cast<const unsigned char*>(addr);
  for (const Entry& entry : g_entries) {
    if (!entry.valid.load(std::memory_order_acquire)) {
      continue;
    }
    const auto begin = static_cast<const unsigned char*>(entry.begin);
    const auto end = static_cast<const unsigned char*>(entry.end);
    if (p >= begin && p < end) {
      return &entry;
    }
  }
  return nullptr;
}

void segv_handler(int, siginfo_t* info, void*) {
  const void* fault_addr = info != nullptr ? info->si_addr : nullptr;
  const Entry* pool = find_pool_by_address(fault_addr);

  write_cstr(STDERR_FILENO, "SIGSEGV: ");
  if (pool != nullptr && pool->name != nullptr) {
    write_cstr(STDERR_FILENO, "pool '");
    write_cstr(STDERR_FILENO, pool->name);
    write_cstr(STDERR_FILENO, "' overflow at ");
  } else {
    write_cstr(STDERR_FILENO, "unknown pool overflow at ");
  }
  write_hex_ptr(STDERR_FILENO, fault_addr);
  write_cstr(STDERR_FILENO, "\n");

  struct sigaction action {};
  action.sa_handler = SIG_DFL;
  sigemptyset(&action.sa_mask);
  sigaction(SIGSEGV, &action, nullptr);
  raise(SIGSEGV);
}

}  // namespace

void register_pool(const char* name, const void* begin, const void* end) {
  for (Entry& entry : g_entries) {
    bool expected = false;
    if (entry.valid.compare_exchange_strong(expected, true,
                                            std::memory_order_release)) {
      entry.name = name;
      entry.begin = begin;
      entry.end = end;
      return;
    }
  }
  const char msg[] = "pool_registry: registry full\n";
  (void)::write(STDERR_FILENO, msg, sizeof(msg) - 1);
  _exit(EXIT_FAILURE);
}

void unregister_pool(const void* begin) {
  for (Entry& entry : g_entries) {
    if (entry.valid.load(std::memory_order_acquire) && entry.begin == begin) {
      entry.valid.store(false, std::memory_order_release);
      entry.name = nullptr;
      entry.begin = nullptr;
      entry.end = nullptr;
      return;
    }
  }
}

void install_segv_handler() {
  bool expected = false;
  if (!g_handler_installed.compare_exchange_strong(expected, true)) {
    return;
  }

  struct sigaction action {};
  action.sa_flags = SA_SIGINFO;
  action.sa_sigaction = segv_handler;
  sigemptyset(&action.sa_mask);
  if (sigaction(SIGSEGV, &action, nullptr) != 0) {
    perror("sigaction(SIGSEGV)");
    _exit(EXIT_FAILURE);
  }
}

}  // namespace pool_registry
