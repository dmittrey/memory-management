#include "safe_read.h"

#include <csignal>
#include <cstdlib>
#include <setjmp.h>

namespace {

static sigjmp_buf g_env;
static volatile sig_atomic_t g_in_safe_read = 0;

static void fault_handler(int signum) {
  (void)signum;
  if (g_in_safe_read) {
    siglongjmp(g_env, 1);
  }

  struct sigaction action {};
  action.sa_handler = SIG_DFL;
  sigemptyset(&action.sa_mask);
  sigaction(signum, &action, nullptr);
  raise(signum);
}

static void install_handlers() {
  struct sigaction action {};
  action.sa_handler = fault_handler;
  sigemptyset(&action.sa_mask);
  action.sa_flags = SA_NODEFER;

  if (sigaction(SIGSEGV, &action, nullptr) != 0) {
    std::abort();
  }
  if (sigaction(SIGBUS, &action, nullptr) != 0) {
    std::abort();
  }
}

struct HandlerInstaller {
  HandlerInstaller() { install_handlers(); }
};

static void ensure_handlers_installed() {
  static const HandlerInstaller installer;
  (void)installer;
}

}  // namespace

std::optional<uint8_t> safe_read_uint8(const uint8_t* p) {
  ensure_handlers_installed();

  std::optional<uint8_t> result;
  g_in_safe_read = 1;

  if (sigsetjmp(g_env, 1) == 0) {
    const volatile uint8_t* vp = p;
    result = *vp;
  }

  g_in_safe_read = 0;
  return result;
}
