#pragma once

#include <atomic>
#include <cstddef>

class LockFreePool {
 public:
  LockFreePool(std::size_t usable_size, const char* name);
  ~LockFreePool();

  LockFreePool(const LockFreePool&) = delete;
  LockFreePool& operator=(const LockFreePool&) = delete;

  void* allocate(std::size_t size, std::size_t alignment);

 private:
  static std::size_t round_up(std::size_t value, std::size_t alignment);

  const char* name_;
  void* mapping_;
  std::size_t mapping_size_;
  unsigned char* bottom_;
  std::atomic<unsigned char*> current_;
  unsigned char* top_;
  std::size_t page_size_;
};
