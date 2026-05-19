#include "pool.h"

#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <unistd.h>

#include <sys/mman.h>

#ifndef MAP_ANON
#define MAP_ANON MAP_ANONYMOUS
#endif

std::size_t Pool::round_up(std::size_t value, std::size_t alignment) {
  return (value + alignment - 1) & ~(alignment - 1);
}

Pool::Pool(std::size_t usable_size)
    : mapping_(nullptr),
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
  current_ = top_;
}

Pool::~Pool() {
  if (mapping_ != nullptr && mapping_ != MAP_FAILED) {
    munmap(mapping_, mapping_size_);
  }
}

void* Pool::allocate(std::size_t size, std::size_t alignment) {
  if (alignment == 0 || (alignment & (alignment - 1)) != 0) {
    std::cerr << "Pool::allocate: alignment must be a power of two\n";
    std::exit(EXIT_FAILURE);
  }

  uintptr_t cur = reinterpret_cast<uintptr_t>(current_);
  cur -= size;
  cur &= ~(static_cast<uintptr_t>(alignment) - 1);

  if (cur < reinterpret_cast<uintptr_t>(bottom_)) {
    std::cerr << "Pool::allocate: out of memory\n";
    std::exit(EXIT_FAILURE);
  }

  current_ = reinterpret_cast<unsigned char*>(cur);
  return current_;
}
