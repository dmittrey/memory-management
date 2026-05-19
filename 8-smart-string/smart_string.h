#pragma once

#include <cstdint>
#include <iosfwd>
#include <string>

class SmartString {
 public:
  SmartString();
  explicit SmartString(const char* s);
  explicit SmartString(std::string s);
  SmartString(const SmartString& other);
  SmartString(SmartString&& other) noexcept;

  SmartString& operator=(const SmartString& other);
  SmartString& operator=(SmartString&& other) noexcept;
  SmartString& operator=(const char* s);

  ~SmartString();

  bool is_unique() const;
  bool is_null() const;
  const std::string& get() const;
  void print(std::ostream& os) const;

  friend void swap(SmartString& a, SmartString& b) noexcept;
  friend std::ostream& operator<<(std::ostream& os, const SmartString& value);

 private:
  static constexpr uintptr_t kUniqueBit = 1;
  mutable uintptr_t encoded_;

  std::string* raw_ptr() const;
  void release();
  void clear_unique_bit() const;
};
