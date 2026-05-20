#define STRING_PTR_DEBUG
#include "smart_string.h"
#undef STRING_PTR_DEBUG

#include <cassert>
#include <cstring>
#include <cstdlib>
#include <iostream>
#include <vector>

using namespace std;

static void print_vector(const char* label, const vector<string_ptr>& values) {
  cout << label << ":\n";
  for (size_t i = 0; i < values.size(); ++i) {
    cout << "  [" << i << "] " << values[i] << '\n';
  }
}

static vector<int> collect_unique_bits(const vector<string_ptr>& values) {
  vector<int> bits;
  bits.reserve(values.size());
  for (const auto& value : values) {
    bits.push_back(value.is_unique() ? 1 : 0);
  }
  return bits;
}

static void bubble_sort(vector<string_ptr>& values) {
  const size_t n = values.size();
  for (size_t i = 0; i + 1 < n; ++i) {
    for (size_t j = 0; j + 1 < n - i; ++j) {
      if (strcmp(*values[j], *values[j + 1]) > 0) {
        swap(values[j], values[j + 1]);
      }
    }
  }
}

static void test_initializations() {
  cout << "--- initializations ---\n";

  string_ptr a;
  string_ptr b("hello");
  string_ptr c(b);
  string_ptr d(std::move(b));

  cout << "a: " << a << '\n';
  cout << "b: " << b << '\n';
  cout << "c: " << c << '\n';
  cout << "d: " << d << '\n';

  assert(!a.is_unique());
  assert(strcmp((*a ? *a : ""), "") == 0);
  assert(!b.is_unique());
  assert(strcmp((*b ? *b : ""), "") == 0);
  assert(!c.is_unique());
  assert(!d.is_unique());
  assert(strcmp(*d, "hello") == 0);
}

static void test_assignments() {
  cout << "--- assignments ---\n";

  string_ptr a;
  string_ptr c("hello");
  string_ptr copy(c);

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

  c = string_ptr("tmp");
  cout << "after c = string_ptr(\"tmp\"):\n";
  cout << "  a: " << a << '\n';
  cout << "  c: " << c << '\n';
  assert(c.is_unique());
  assert(strcmp(*a, "world") == 0);
}

static void test_bubble_sort() {
  cout << "--- bubble sort ---\n";

  vector<string_ptr> values;
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
    assert(strcmp(*values[i], *values[i + 1]) <= 0);
  }
}

int main() {
  test_initializations();
  test_assignments();
  test_bubble_sort();
  return EXIT_SUCCESS;
}
