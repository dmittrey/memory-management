#pragma once

#include "pool_registry.h"

#include <atomic>
#include <cstddef>
#include <cstdlib>
#include <mutex>
#include <stdexcept>
#include <unistd.h>

#include <sys/mman.h>

class BasePool {
 public:
  BasePool(std::size_t capacity, std::size_t max_alloc_size)
      : mapping_(nullptr),
        mapping_size_(0),
        base_(nullptr),
        pool_id_(-1) {
    if (capacity == 0) {
      throw std::invalid_argument("capacity is zero!");
    }
    if (max_alloc_size == 0) {
      throw std::invalid_argument("max alloc size is zero!");
    }

    const std::size_t page_size =
        static_cast<std::size_t>(sysconf(_SC_PAGESIZE));
    if (page_size == static_cast<std::size_t>(-1)) {
      throw std::runtime_error("unable to get _SC_PAGESIZE from system!");
    }

    const std::size_t guard_size = round_up(max_alloc_size, page_size);
    mapping_size_ = round_up(capacity, page_size) + guard_size;

    mapping_ = mmap(nullptr, mapping_size_, PROT_READ | PROT_WRITE,
                    MAP_PRIVATE | MAP_ANON, -1, 0);
    if (mapping_ == MAP_FAILED) {
      throw std::runtime_error("unable to allocate pool via MMAP!");
    }

    if (mprotect(mapping_, guard_size, PROT_NONE) != 0) {
      munmap(mapping_, mapping_size_);
      throw std::runtime_error("unable to protect pool edge!");
    }

    base_ = static_cast<unsigned char*>(mapping_);
    pool_id_ = pool_registry::register_pool(base_, base_ + guard_size);
  }

  ~BasePool() {
    pool_registry::unregister_pool(pool_id_);
    if (mapping_ != nullptr && mapping_ != MAP_FAILED) {
      munmap(mapping_, mapping_size_);
    }
  }

  BasePool(const BasePool&) = delete;
  BasePool& operator=(const BasePool&) = delete;

 protected:
  unsigned char* top() const { return base_ + mapping_size_; }

 private:
  static std::size_t round_up(std::size_t value, std::size_t alignment) {
    return (value + alignment - 1) & ~(alignment - 1);
  }

  void* mapping_;
  std::size_t mapping_size_;
  unsigned char* base_;
  int pool_id_;
};

class Pool : public BasePool {
 public:
  Pool(std::size_t capacity, std::size_t max_alloc_size)
      : BasePool(capacity, max_alloc_size), first_free_(top()) {}

  void* allocate(std::size_t size) { return first_free_ -= size; }

 private:
  unsigned char* first_free_;
};

class LockFreePool : public BasePool {
 public:
  LockFreePool(std::size_t capacity, std::size_t max_alloc_size)
      : BasePool(capacity, max_alloc_size),
        first_free_(top()) {}

  void* allocate(std::size_t size) {
    return first_free_.fetch_sub(size, std::memory_order_relaxed) - size;
  }

 private:
  std::atomic<unsigned char*> first_free_;
};

class MutexPool : public Pool {
 public:
  using Pool::Pool;

  void* allocate(std::size_t size) {
    std::lock_guard<std::mutex> lock(mutex_);
    return Pool::allocate(size);
  }

 private:
  std::mutex mutex_;
};
