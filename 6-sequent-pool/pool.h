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

  Pool(std::size_t capacity, std::size_t max_alloc_size)
    : mapping_(nullptr), mapping_size_(0), guard_size_(0), first_free_(nullptr) {

    if (capacity == 0) {
      throw std::invalid_argument("capacity is zero!"); 
    }
    if (max_alloc_size == 0) {
      throw std::invalid_argument("max alloc size is zero!"); 
    }

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