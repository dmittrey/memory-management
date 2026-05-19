#include <iomanip>
#include <iostream>
#include <new>
#include <stdint.h>
#include <stdlib.h>
#include <sys/resource.h>
#include <sys/time.h>

#ifdef USE_POOL_ALLOCATOR
#include "pool.h"
#endif

using namespace std;

static void get_usage(struct rusage& usage) {
  if (getrusage(RUSAGE_SELF, &usage)) {
    perror("Cannot get usage");
    exit(EXIT_SUCCESS);
  }
}

struct Node {
  Node* next;
  unsigned node_id;
};

#ifdef USE_POOL_ALLOCATOR
static inline Node* create_list(unsigned n, Pool& pool) {
  Node* list = nullptr;
  for (unsigned i = 0; i < n; i++) {
    void* memory = pool.allocate(sizeof(Node), alignof(Node));
    list = new (memory) Node{list, i};
  }
  return list;
}
#else
static inline Node* create_list(unsigned n) {
  Node* list = nullptr;
  for (unsigned i = 0; i < n; i++)
    list = new Node({list, i});
  return list;
}

static inline void delete_list(Node* list) {
  while (list) {
    Node* node = list;
    list = list->next;
    delete node;
  }
}
#endif

static inline void test(unsigned n) {
#ifdef USE_POOL_ALLOCATOR
  cout << "Mode: downward mmap pool\n";
#else
  cout << "Mode: default new/delete\n";
#endif

  struct rusage start, finish;
  get_usage(start);

#ifdef USE_POOL_ALLOCATOR
  Pool pool(n * sizeof(Node));
  create_list(n, pool);
#else
  delete_list(create_list(n));
#endif

  get_usage(finish);

  struct timeval diff;
  timersub(&finish.ru_utime, &start.ru_utime, &diff);
  uint64_t time_used = diff.tv_sec * 1000000 + diff.tv_usec;
  cout << "Time used: " << time_used << " usec\n";

  uint64_t mem_used = (finish.ru_maxrss - start.ru_maxrss) * 1024;
  cout << "Memory used: " << mem_used << " bytes\n";

  auto mem_required = n * sizeof(Node);
  auto overhead = (mem_used - mem_required) * double(100) / mem_used;
  cout << "Overhead: " << std::fixed << std::setw(4) << std::setprecision(1)
       << overhead << "%\n";
}

int main(const int argc, const char* argv[]) {
  test(10000000);
  return EXIT_SUCCESS;
}
