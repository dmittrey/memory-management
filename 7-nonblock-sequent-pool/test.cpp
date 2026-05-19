#include <iomanip>
#include <iostream>
#include <new>
#include <stdint.h>
#include <stdlib.h>
#include <string>
#include <sys/resource.h>
#include <sys/time.h>
#include <thread>
#include <vector>

#include "pool_registry.h"

#if defined(USE_MUTEX_POOL)
#include "mutex_pool.h"
#elif defined(USE_LOCKFREE_POOL)
#include "lockfree_pool.h"
#elif defined(USE_THREAD_LOCAL_POOL)
#include "pool.h"
#endif

using namespace std;

static constexpr unsigned kListLength = 10000000;
static constexpr unsigned kThreadCount = 16;

static void get_usage(struct rusage& usage) {
  if (getrusage(RUSAGE_SELF, &usage)) {
    perror("Cannot get usage");
    exit(EXIT_FAILURE);
  }
}

static uint64_t elapsed_usec(const struct rusage& start, const struct rusage& finish) {
  struct timeval ut_diff {};
  struct timeval st_diff {};
  timersub(&finish.ru_utime, &start.ru_utime, &ut_diff);
  timersub(&finish.ru_stime, &start.ru_stime, &st_diff);
  return static_cast<uint64_t>(ut_diff.tv_sec) * 1000000 + ut_diff.tv_usec +
         static_cast<uint64_t>(st_diff.tv_sec) * 1000000 + st_diff.tv_usec;
}

struct Node {
  Node* next;
  unsigned node_id;
};

#if defined(USE_MUTEX_POOL)
static MutexPool* g_pool = nullptr;

static inline Node* create_list(unsigned n, MutexPool& pool) {
  Node* list = nullptr;
  for (unsigned i = 0; i < n; i++) {
    void* memory = pool.allocate(sizeof(Node), alignof(Node));
    list = new (memory) Node{list, i};
  }
  return list;
}
#elif defined(USE_LOCKFREE_POOL)
static LockFreePool* g_pool = nullptr;

static inline Node* create_list(unsigned n, LockFreePool& pool) {
  Node* list = nullptr;
  for (unsigned i = 0; i < n; i++) {
    void* memory = pool.allocate(sizeof(Node), alignof(Node));
    list = new (memory) Node{list, i};
  }
  return list;
}
#elif defined(USE_THREAD_LOCAL_POOL)
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
  for (unsigned i = 0; i < n; i++) {
    list = new Node({list, i});
  }
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

static void thread_fn(unsigned tid) {
#if defined(USE_MUTEX_POOL)
  (void)create_list(kListLength, *g_pool);
#elif defined(USE_LOCKFREE_POOL)
  (void)create_list(kListLength, *g_pool);
#elif defined(USE_THREAD_LOCAL_POOL)
  const string pool_name = "tls-" + to_string(tid);
  Pool local_pool(kListLength * sizeof(Node), pool_name.c_str());
  (void)create_list(kListLength, local_pool);
#else
  delete_list(create_list(kListLength));
#endif
}

static void print_mode() {
#if defined(USE_MUTEX_POOL)
  cout << "Mode: global mutex pool\n";
#elif defined(USE_LOCKFREE_POOL)
  cout << "Mode: global lock-free pool\n";
#elif defined(USE_THREAD_LOCAL_POOL)
  cout << "Mode: thread-local pools\n";
#else
  cout << "Mode: default new/delete\n";
#endif
}

int main() {
  pool_registry::install_segv_handler();

#if defined(USE_MUTEX_POOL)
  MutexPool global_pool(kThreadCount * kListLength * sizeof(Node), "global-mutex");
  g_pool = &global_pool;
#elif defined(USE_LOCKFREE_POOL)
  LockFreePool global_pool(kThreadCount * kListLength * sizeof(Node), "global-lockfree");
  g_pool = &global_pool;
#endif

  print_mode();

  struct rusage start {};
  struct rusage finish {};
  get_usage(start);

  vector<thread> threads;
  threads.reserve(kThreadCount);
  for (unsigned i = 0; i < kThreadCount; ++i) {
    threads.emplace_back(thread_fn, i);
  }
  for (auto& t : threads) {
    t.join();
  }

  get_usage(finish);

  const uint64_t time_used_usec = elapsed_usec(start, finish);
  const double time_used_sec =
      static_cast<double>(time_used_usec) / 1'000'000.0;
  cout << "Time used: " << fixed << setprecision(3) << time_used_sec
       << " sec\n";

  const uint64_t mem_used =
      static_cast<uint64_t>(finish.ru_maxrss - start.ru_maxrss) * 1024;
  const double mem_used_gb =
      static_cast<double>(mem_used) / (1024.0 * 1024.0 * 1024.0);
  cout << "Memory used: " << fixed << setprecision(3) << mem_used_gb << " GB\n";

#if defined(USE_THREAD_LOCAL_POOL)
  const auto mem_required =
      static_cast<uint64_t>(kListLength) * sizeof(Node);
#else
  const auto mem_required =
      static_cast<uint64_t>(kThreadCount) * kListLength * sizeof(Node);
#endif
  if (mem_used >= mem_required) {
    const auto overhead =
        static_cast<double>(mem_used - mem_required) * 100.0 / mem_used;
    cout << "Overhead: " << fixed << setw(4) << setprecision(1) << overhead
         << "%\n";
  }

  return EXIT_SUCCESS;
}
