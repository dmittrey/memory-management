#include "pool.h"

#include <cstdio>

static int find_pool_id(const void* addr) {
    const Pool* pool = Pool::active_;
    if (pool != nullptr && pool->contains_guard(addr)) {
        return 0;
    }
    return -1;
}
  
static void pool_overflow_handler(int sig, siginfo_t* info, void*) {
    const void* fault_addr = info != nullptr ? info->si_addr : nullptr;
    if (find_pool_id(fault_addr) != -1) {
      static const char msg[] = "SIGSEGV: pool overflow, pool id = 0\n";
      (void)::write(STDERR_FILENO, msg, sizeof(msg) - 1);
      _exit(EXIT_FAILURE);
    }
    signal(sig, SIG_DFL);
    raise(sig);
}

struct HandlerInstaller {
    HandlerInstaller() {
        struct sigaction action {};
        action.sa_flags = SA_SIGINFO;
        action.sa_sigaction = pool_overflow_handler;
        sigemptyset(&action.sa_mask);
        
        if (sigaction(SIGSEGV, &action, nullptr) != 0) {
            perror("sigaction(SIGSEGV)");
            _exit(EXIT_FAILURE);
        }
    }
};
  
#ifdef USE_POOL_ALLOCATOR
    const HandlerInstaller g_installer{};
#endif