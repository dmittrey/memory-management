#pragma once

#include <cstddef>
#include <cstdlib>
#include <unistd.h>

#include <sys/mman.h>

class Pool {
 public:
  Pool(std::size_t capacity, std::size_t max_alloc_size)
      : mapping_(nullptr),
        mapping_size_(0),
        guard_size_(0),
        first_free_(nullptr) {
    const std::size_t page_size =
        static_cast<std::size_t>(sysconf(_SC_PAGESIZE));
    if (page_size == static_cast<std::size_t>(-1)) {
      perror("sysconf(_SC_PAGESIZE)");
      std::exit(EXIT_FAILURE);
    }

    guard_size_ = round_up(max_alloc_size, page_size);
    if (guard_size_ == 0) {
      guard_size_ = page_size;
    }

    const std::size_t rounded_capacity = round_up(capacity, page_size);
    mapping_size_ = rounded_capacity + guard_size_;

    mapping_ = mmap(nullptr, mapping_size_, PROT_READ | PROT_WRITE,
                    MAP_PRIVATE | MAP_ANON, -1, 0);
    if (mapping_ == MAP_FAILED) {
      perror("mmap");
      std::exit(EXIT_FAILURE);
    }

    if (mprotect(mapping_, guard_size_, PROT_NONE) != 0) {
      perror("mprotect");
      munmap(mapping_, mapping_size_);
      std::exit(EXIT_FAILURE);
    }

    first_free_ = static_cast<unsigned char*>(mapping_) + mapping_size_;
  }

  ~Pool() {
    if (mapping_ != nullptr && mapping_ != MAP_FAILED) {
      munmap(mapping_, mapping_size_);
    }
  }

  Pool(const Pool&) = delete;
  Pool& operator=(const Pool&) = delete;

  void* allocate(std::size_t size) { return first_free_ -= size; }

 private:
  static std::size_t round_up(std::size_t value, std::size_t alignment) {
    return (value + alignment - 1) & ~(alignment - 1);
  }

  void* mapping_;
  std::size_t mapping_size_;
  std::size_t guard_size_;
  unsigned char* first_free_;
};
