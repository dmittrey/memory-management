#include "smart_string.h"

#include <cassert>
#include <cstdlib>
#include <iostream>
#include <vector>

using namespace std;

static void print_mode() {
#ifdef DEBUG_TRACE
  cout << "Mode: debug (release tracing enabled)\n";
#else
  cout << "Mode: release\n";
#endif
}

static void print_vector(const char* label, const vector<SmartString>& values) {
  cout << label << ":\n";
  for (size_t i = 0; i < values.size(); ++i) {
    cout << "  [" << i << "] " << values[i] << '\n';
  }
}

static vector<int> collect_unique_bits(const vector<SmartString>& values) {
  vector<int> bits;
  bits.reserve(values.size());
  for (const auto& value : values) {
    bits.push_back(value.is_unique() ? 1 : 0);
  }
  return bits;
}

static void bubble_sort(vector<SmartString>& values) {
  const size_t n = values.size();
  for (size_t i = 0; i + 1 < n; ++i) {
    for (size_t j = 0; j + 1 < n - i; ++j) {
      if (values[j].get() > values[j + 1].get()) {
        swap(values[j], values[j + 1]);
      }
    }
  }
}

static void test_initializations() {
  cout << "--- initializations ---\n";

  SmartString a;
  SmartString b("hello");
  SmartString c(b);
  SmartString d(std::move(b));

  cout << "a: " << a << '\n';
  cout << "b: " << b << '\n';
  cout << "c: " << c << '\n';
  cout << "d: " << d << '\n';

  assert(a.is_null());
  assert(!a.is_unique());
  assert(b.is_null());
  assert(!c.is_unique());
  assert(!d.is_unique());
  assert(d.get() == "hello");
}

static void test_assignments() {
  cout << "--- assignments ---\n";

  SmartString a;
  SmartString c("hello");
  SmartString copy(c);

  cout << "before:\n";
  cout << "  a: " << a << '\n';
  cout << "  c: " << c << '\n';
  cout << "  copy: " << copy << '\n';

  c = "world";
  cout << "after c = \"world\": " << c << '\n';
  assert(c.is_unique());

  a = c;
  cout << "after a = c:\n";
  cout << "  a: " << a << '\n';
  cout << "  c: " << c << '\n';
  assert(!a.is_unique());
  assert(!c.is_unique());

  c = SmartString("tmp");
  cout << "after c = SmartString(\"tmp\"):\n";
  cout << "  a: " << a << '\n';
  cout << "  c: " << c << '\n';
  assert(c.is_unique());
  assert(a.get() == "world");
}

static void test_bubble_sort() {
  cout << "--- bubble sort ---\n";

  vector<SmartString> values;
  values.emplace_back("delta");
  values.emplace_back("alpha");
  values.emplace_back("charlie");
  values.emplace_back("bravo");
  values.emplace_back("alpha");
  values.emplace_back("echo");

  print_vector("before", values);
  const vector<int> bits_before = collect_unique_bits(values);

  bubble_sort(values);

  print_vector("after", values);
  const vector<int> bits_after = collect_unique_bits(values);

  assert(bits_before == bits_after);

  for (size_t i = 0; i + 1 < values.size(); ++i) {
    assert(values[i].get() <= values[i + 1].get());
  }
}

int main() {
  print_mode();
  test_initializations();
  test_assignments();
  test_bubble_sort();
  return EXIT_SUCCESS;
}
