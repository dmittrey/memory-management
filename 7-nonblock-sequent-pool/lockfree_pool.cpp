#include "lockfree_pool.h"

#include "pool_registry.h"

#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <unistd.h>

#include <sys/mman.h>

#ifndef MAP_ANON
#define MAP_ANON MAP_ANONYMOUS
#endif

std::size_t LockFreePool::round_up(std::size_t value, std::size_t alignment) {
  return (value + alignment - 1) & ~(alignment - 1);
}

LockFreePool::LockFreePool(std::size_t usable_size, const char* name)
    : name_(name),
      mapping_(nullptr),
      mapping_size_(0),
      bottom_(nullptr),
      current_(nullptr),
      top_(nullptr),
      page_size_(0) {
  page_size_ = static_cast<std::size_t>(sysconf(_SC_PAGESIZE));
  if (page_size_ == static_cast<std::size_t>(-1)) {
    perror("sysconf(_SC_PAGESIZE)");
    std::exit(EXIT_FAILURE);
  }

  const std::size_t usable_pages = round_up(usable_size, page_size_);
  mapping_size_ = page_size_ + usable_pages;

  mapping_ = mmap(nullptr, mapping_size_, PROT_READ | PROT_WRITE,
                  MAP_PRIVATE | MAP_ANON, -1, 0);
  if (mapping_ == MAP_FAILED) {
    perror("mmap");
    std::exit(EXIT_FAILURE);
  }

  if (mprotect(mapping_, page_size_, PROT_NONE) != 0) {
    perror("mprotect");
    munmap(mapping_, mapping_size_);
    std::exit(EXIT_FAILURE);
  }

  bottom_ = static_cast<unsigned char*>(mapping_) + page_size_;
  top_ = static_cast<unsigned char*>(mapping_) + mapping_size_;
  current_.store(top_, std::memory_order_relaxed);

  pool_registry::register_pool(name_, mapping_,
                               static_cast<unsigned char*>(mapping_) + page_size_);
}

LockFreePool::~LockFreePool() {
  if (mapping_ != nullptr && mapping_ != MAP_FAILED) {
    pool_registry::unregister_pool(mapping_);
    munmap(mapping_, mapping_size_);
  }
}

void* LockFreePool::allocate(std::size_t size, std::size_t alignment) {
  if (alignment == 0 || (alignment & (alignment - 1)) != 0) {
    std::cerr << "LockFreePool::allocate: alignment must be a power of two\n";
    std::exit(EXIT_FAILURE);
  }

  unsigned char* cur = current_.load(std::memory_order_relaxed);
  unsigned char* desired = nullptr;
  do {
    uintptr_t p = reinterpret_cast<uintptr_t>(cur) - size;
    p &= ~(static_cast<uintptr_t>(alignment) - 1);
    desired = reinterpret_cast<unsigned char*>(p);
  } while (!current_.compare_exchange_weak(cur, desired, std::memory_order_relaxed,
                                         std::memory_order_relaxed));
  return desired;
}
