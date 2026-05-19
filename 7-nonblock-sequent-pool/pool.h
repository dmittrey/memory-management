#pragma once

#include <cstddef>

class Pool {
 public:
  Pool(std::size_t usable_size, const char* name);
  ~Pool();

  Pool(const Pool&) = delete;
  Pool& operator=(const Pool&) = delete;

  void* allocate(std::size_t size, std::size_t alignment);

 private:
  static std::size_t round_up(std::size_t value, std::size_t alignment);

  const char* name_;
  void* mapping_;
  std::size_t mapping_size_;
  unsigned char* bottom_;
  unsigned char* current_;
  unsigned char* top_;
  std::size_t page_size_;
};
