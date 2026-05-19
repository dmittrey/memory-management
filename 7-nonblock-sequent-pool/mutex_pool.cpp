#include "mutex_pool.h"

MutexPool::MutexPool(std::size_t usable_size, const char* name)
    : pool_(usable_size, name) {}

void* MutexPool::allocate(std::size_t size, std::size_t alignment) {
  std::lock_guard<std::mutex> lock(mutex_);
  return pool_.allocate(size, alignment);
}
