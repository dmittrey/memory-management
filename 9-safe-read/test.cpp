#include "safe_read.h"

#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <optional>
#include <unistd.h>

#include <sys/mman.h>

#ifndef MAP_ANON
#define MAP_ANON MAP_ANONYMOUS
#endif

using namespace std;

static void print_mode() {
#ifdef DEBUG_TRACE
  cout << "Mode: debug\n";
#else
  cout << "Mode: release\n";
#endif
}

static void print_result(const char* label, const optional<uint8_t>& value) {
  cout << label << ": ";
  if (value.has_value()) {
    cout << "Some(" << static_cast<unsigned>(*value) << ")\n";
  } else {
    cout << "nullopt\n";
  }
}

static void test_nullptr() {
  cout << "--- nullptr ---\n";
  const optional<uint8_t> value = safe_read_uint8(nullptr);
  print_result("safe_read_uint8(nullptr)", value);
  assert(!value.has_value());
}

static void test_valid_local() {
  cout << "--- valid local ---\n";
  const uint8_t byte = 0x42;
  const optional<uint8_t> value = safe_read_uint8(&byte);
  print_result("safe_read_uint8(&local)", value);
  assert(value.has_value());
  assert(*value == 0x42);
}

static void test_invalid_address() {
  cout << "--- invalid address ---\n";
  const auto* bad = reinterpret_cast<const uint8_t*>(0x1);
  const optional<uint8_t> value = safe_read_uint8(bad);
  print_result("safe_read_uint8(0x1)", value);
  assert(!value.has_value());
}

static void test_mprotect() {
  cout << "--- mprotect ---\n";

  const size_t page_size = static_cast<size_t>(sysconf(_SC_PAGESIZE));
  assert(page_size != static_cast<size_t>(-1));

  void* mapping = mmap(nullptr, 2 * page_size, PROT_READ | PROT_WRITE,
                       MAP_PRIVATE | MAP_ANON, -1, 0);
  assert(mapping != MAP_FAILED);

  auto* base = static_cast<uint8_t*>(mapping);
  base[page_size] = 0x7E;

  assert(mprotect(mapping, page_size, PROT_NONE) == 0);

  const optional<uint8_t> protected_value = safe_read_uint8(base);
  print_result("safe_read_uint8(PROT_NONE page)", protected_value);
  assert(!protected_value.has_value());

  const optional<uint8_t> readable_value = safe_read_uint8(base + page_size);
  print_result("safe_read_uint8(PROT_READ page)", readable_value);
  assert(readable_value.has_value());
  assert(*readable_value == 0x7E);

  munmap(mapping, 2 * page_size);
}

static void test_after_fault() {
  cout << "--- after fault ---\n";

  const auto* bad = reinterpret_cast<const uint8_t*>(0x1);
  const optional<uint8_t> fault_value = safe_read_uint8(bad);
  print_result("safe_read_uint8(0x1)", fault_value);
  assert(!fault_value.has_value());

  const uint8_t byte = 0x55;
  const optional<uint8_t> ok_value = safe_read_uint8(&byte);
  print_result("safe_read_uint8(&local after fault)", ok_value);
  assert(ok_value.has_value());
  assert(*ok_value == 0x55);
}

int main() {
  print_mode();
  test_nullptr();
  test_valid_local();
  test_invalid_address();
  test_mprotect();
  test_after_fault();
  return EXIT_SUCCESS;
}
