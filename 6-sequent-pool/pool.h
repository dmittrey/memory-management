#pragma once

#include <cstddef>
#include <cstdlib>
#include <stdexcept>
#include <unistd.h>

#include <signal.h>
#include <sys/mman.h>

class Pool {
 public:
  inline static Pool* active_ = nullptr;

  Pool(std::size_t capacity, std::size_t max_alloc_size);
  ~Pool() {
    if (active_ == this) {
      active_ = nullptr;
    }
    munmap(mapping_, mapping_size_);
  }

  Pool(const Pool&) = delete;
  Pool& operator=(const Pool&) = delete;

  void* allocate(std::size_t size) { return first_free_ -= size; }

  bool contains_guard(const void* addr) const {
    const auto p = static_cast<const unsigned char*>(addr);
    const auto begin = static_cast<const unsigned char*>(mapping_);
    return p >= begin && p < begin + guard_size_;
  }

 private:
  static std::size_t round_up(std::size_t value, std::size_t alignment) {
    return (value + alignment - 1) & ~(alignment - 1);
  }

  void* mapping_;
  std::size_t mapping_size_;
  std::size_t guard_size_;
  unsigned char* first_free_;
};