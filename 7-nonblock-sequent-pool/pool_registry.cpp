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

constexpr int kMaxPools = 128;
const void* const kClaimSentinel =
    reinterpret_cast<const void*>(uintptr_t(1));

struct Entry {
  std::atomic<const void*> guard_start{nullptr};
  std::atomic<const void*> guard_end{nullptr};
};

std::array<Entry, kMaxPools> g_entries{};
struct sigaction g_prev_handler {};

void write_cstr(int fd, const char* s) {
  if (s == nullptr) {
    return;
  }
  const std::size_t len = std::strlen(s);
  if (len > 0) {
    (void)::write(fd, s, len);
  }
}

void write_int(int fd, int value) {
  char buf[16];
  int pos = 0;
  if (value == 0) {
    buf[pos++] = '0';
  } else {
    char digits[16];
    int count = 0;
    while (value > 0) {
      digits[count++] = static_cast<char>('0' + value % 10);
      value /= 10;
    }
    while (count > 0) {
      buf[pos++] = digits[--count];
    }
  }
  (void)::write(fd, buf, static_cast<std::size_t>(pos));
}

int find_pool_id(const void* addr) {
  const auto p = static_cast<const unsigned char*>(addr);
  for (int i = 0; i < kMaxPools; ++i) {
    const void* start =
        g_entries[i].guard_start.load(std::memory_order_acquire);
    if (start == nullptr || start == kClaimSentinel) {
      continue;
    }
    const void* end =
        g_entries[i].guard_end.load(std::memory_order_relaxed);
    const auto begin = static_cast<const unsigned char*>(start);
    const auto guard_end = static_cast<const unsigned char*>(end);
    if (p >= begin && p < guard_end) {
      return i;
    }
  }
  return -1;
}

void dispatch_prev_handler(int sig, siginfo_t* info, void* ctx) {
  if (g_prev_handler.sa_flags & SA_SIGINFO) {
    g_prev_handler.sa_sigaction(sig, info, ctx);
    return;
  }

  if (g_prev_handler.sa_handler == SIG_DFL) {
    struct sigaction action {};
    action.sa_handler = SIG_DFL;
    sigemptyset(&action.sa_mask);
    sigaction(sig, &action, nullptr);
    raise(sig);
    return;
  }

  if (g_prev_handler.sa_handler == SIG_IGN) {
    return;
  }

  g_prev_handler.sa_handler(sig);
}

void segv_handler(int sig, siginfo_t* info, void* ctx) {
  const void* fault_addr = info != nullptr ? info->si_addr : nullptr;
  const int pool_id = find_pool_id(fault_addr);

  if (pool_id != -1) {
    write_cstr(STDERR_FILENO, "SIGSEGV: pool overflow, pool id = ");
    write_int(STDERR_FILENO, pool_id);
    write_cstr(STDERR_FILENO, "\n");
    _exit(EXIT_FAILURE);
  }

  dispatch_prev_handler(sig, info, ctx);
}

struct HandlerInstaller {
  HandlerInstaller() {
    struct sigaction action {};
    action.sa_flags = SA_SIGINFO;
    action.sa_sigaction = segv_handler;
    sigemptyset(&action.sa_mask);
    if (sigaction(SIGSEGV, &action, &g_prev_handler) != 0) {
      perror("sigaction(SIGSEGV)");
      _exit(EXIT_FAILURE);
    }
  }
};

const HandlerInstaller g_installer{};

}  // namespace

int register_pool(const void* guard_start, const void* guard_end) {
  for (int i = 0; i < kMaxPools; ++i) {
    const void* expected = nullptr;
    if (g_entries[i].guard_start.compare_exchange_strong(
            expected, kClaimSentinel, std::memory_order_relaxed,
            std::memory_order_relaxed)) {
      g_entries[i].guard_end.store(guard_end, std::memory_order_relaxed);
      g_entries[i].guard_start.store(guard_start, std::memory_order_release);
      return i;
    }
  }

  const char msg[] = "pool_registry: registry full\n";
  (void)::write(STDERR_FILENO, msg, sizeof(msg) - 1);
  _exit(EXIT_FAILURE);
}

void unregister_pool(int id) {
  if (id < 0 || id >= kMaxPools) {
    return;
  }
  // Deactivate slot so find_pool_id skips it and register_pool can reuse it.
  g_entries[id].guard_start.store(nullptr, std::memory_order_relaxed);
}

}  // namespace pool_registry
