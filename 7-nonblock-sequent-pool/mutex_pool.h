#pragma once

#include "pool.h"

#include <cstddef>
#include <mutex>

class MutexPool {
 public:
  MutexPool(std::size_t usable_size, const char* name);
  ~MutexPool() = default;

  MutexPool(const MutexPool&) = delete;
  MutexPool& operator=(const MutexPool&) = delete;

  void* allocate(std::size_t size, std::size_t alignment);

 private:
  Pool pool_;
  std::mutex mutex_;
};
