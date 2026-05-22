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

namespace {

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

}

Pool::Pool(std::size_t capacity, std::size_t max_alloc_size)
    : mapping_(nullptr), mapping_size_(0), guard_size_(0), first_free_(nullptr) {

    if (capacity == 0) {
      throw std::invalid_argument("capacity is zero!"); 
    }
    if (max_alloc_size == 0) {
      throw std::invalid_argument("max alloc size is zero!"); 
    }

    static HandlerInstaller handler_installer;

    const std::size_t page_size = static_cast<std::size_t>(sysconf(_SC_PAGESIZE));
    if (page_size == static_cast<std::size_t>(-1)) {
      throw std::runtime_error("unable to get _SC_PAGESIZE from system!");
    }

    guard_size_ = round_up(max_alloc_size, page_size);
    mapping_size_ = round_up(capacity, page_size) + guard_size_;

    mapping_ = mmap(nullptr, mapping_size_, PROT_READ | PROT_WRITE,
                    MAP_PRIVATE | MAP_ANON, -1, 0);
    if (mapping_ == MAP_FAILED) {
      throw std::runtime_error("unable to allocate pool via MMAP!");
    }

    if (mprotect(mapping_, guard_size_, PROT_NONE) != 0) {
      munmap(mapping_, mapping_size_);
      throw std::runtime_error("unable to protect pool edge!");
    }

    first_free_ = static_cast<unsigned char*>(mapping_) + mapping_size_;
    active_ = this;
}