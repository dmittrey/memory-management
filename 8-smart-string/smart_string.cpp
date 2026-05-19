#include "smart_string.h"

#include <cassert>
#include <iostream>
#include <utility>

static_assert(alignof(std::string) >= 2,
              "std::string must be aligned so the low bit of its address is free");

std::string* SmartString::raw_ptr() const {
  return reinterpret_cast<std::string*>(encoded_ & ~kUniqueBit);
}

void SmartString::release() {
  if (encoded_ == 0) {
    return;
  }

  if ((encoded_ & kUniqueBit) != 0) {
#ifdef DEBUG_TRACE
    std::cerr << "[trace] release \"" << *raw_ptr() << "\" @ " << raw_ptr()
              << '\n';
#endif
    delete raw_ptr();
  }

  encoded_ = 0;
}

void SmartString::clear_unique_bit() const {
  encoded_ &= ~kUniqueBit;
}

SmartString::SmartString() : encoded_(0) {}

SmartString::SmartString(const char* s)
    : encoded_(reinterpret_cast<uintptr_t>(new std::string(s)) | kUniqueBit) {}

SmartString::SmartString(std::string s)
    : encoded_(reinterpret_cast<uintptr_t>(new std::string(std::move(s))) |
               kUniqueBit) {}

SmartString::SmartString(const SmartString& other) : encoded_(other.encoded_) {
  clear_unique_bit();
  other.clear_unique_bit();
}

SmartString::SmartString(SmartString&& other) noexcept
    : encoded_(other.encoded_) {
  other.encoded_ = 0;
}

SmartString& SmartString::operator=(const SmartString& other) {
  if (this == &other) {
    return *this;
  }

  release();
  encoded_ = other.encoded_;
  clear_unique_bit();
  other.clear_unique_bit();
  return *this;
}

SmartString& SmartString::operator=(SmartString&& other) noexcept {
  if (this == &other) {
    return *this;
  }

  release();
  encoded_ = other.encoded_;
  other.encoded_ = 0;
  return *this;
}

SmartString& SmartString::operator=(const char* s) {
  release();
  encoded_ = reinterpret_cast<uintptr_t>(new std::string(s)) | kUniqueBit;
  return *this;
}

SmartString::~SmartString() {
  release();
}

bool SmartString::is_unique() const {
  return (encoded_ & kUniqueBit) != 0;
}

bool SmartString::is_null() const {
  return encoded_ == 0;
}

const std::string& SmartString::get() const {
  assert(!is_null());
  return *raw_ptr();
}

void SmartString::print(std::ostream& os) const {
  if (is_null()) {
    os << "{null}";
    return;
  }

  os << "{unique=" << (is_unique() ? 1 : 0) << ", \"" << *raw_ptr() << "\"}";
}

void swap(SmartString& a, SmartString& b) noexcept {
  std::swap(a.encoded_, b.encoded_);
}

std::ostream& operator<<(std::ostream& os, const SmartString& value) {
  value.print(os);
  return os;
}
