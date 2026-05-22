#include <iomanip>
#include <iostream>
#include <stdint.h>
#include <stdlib.h>
#include <sys/resource.h>
#include <sys/time.h>
#include <thread>
#include <vector>

#if defined(USE_MUTEX_POOL) || defined(USE_LOCKFREE_POOL) || \
    defined(USE_THREAD_LOCAL_POOL)
#include "pool.h"
#endif

using namespace std;

const unsigned NUM_THREADS = 16;
const unsigned N = 10000000;

static void get_usage(struct rusage& usage) {
  if (getrusage(RUSAGE_SELF, &usage)) {
    perror("Cannot get usage");
    exit(EXIT_FAILURE);
  }
}

struct Node {
  Node* next;
  unsigned node_id;
};

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

#if defined(USE_MUTEX_POOL) || defined(USE_LOCKFREE_POOL) || \
    defined(USE_THREAD_LOCAL_POOL)
template <typename PoolT>
static inline Node* create_list_using_pool(unsigned n, PoolT& p) {
  Node* list = nullptr;
  for (unsigned i = 0; i < n; i++) {
    Node* new_node = static_cast<Node*>(p.allocate(sizeof(Node)));
    new_node->node_id = i;
    new_node->next = list;
    list = new_node;
  }
  return list;
}
#endif

static void print_stats(struct rusage& start, struct rusage& finish, unsigned n) {
  struct timeval diff;
  timersub(&finish.ru_utime, &start.ru_utime, &diff);
  const double time_used_sec =
      static_cast<double>(diff.tv_sec) +
      static_cast<double>(diff.tv_usec) / 1e6;
  cout << "Time used: " << std::fixed << std::setprecision(3) << time_used_sec
       << " sec\n";

#ifdef __APPLE__
  const uint64_t mem_used =
      static_cast<uint64_t>(finish.ru_maxrss - start.ru_maxrss);
#else
  const uint64_t mem_used =
      static_cast<uint64_t>(finish.ru_maxrss - start.ru_maxrss) * 1024;
#endif
  const double mem_used_gb =
      static_cast<double>(mem_used) / (1024.0 * 1024.0 * 1024.0);
  cout << "Memory used: " << std::fixed << std::setprecision(3) << mem_used_gb
       << " GB\n";

  const auto mem_required = static_cast<uint64_t>(n) * sizeof(Node);
  const auto overhead =
      (double(mem_used) - double(mem_required)) * double(100) / mem_used;
  cout << "Overhead: " << std::fixed << std::setw(4) << std::setprecision(3)
       << overhead << "%\n";
}

#if defined(USE_MUTEX_POOL)
static void thread_global_mutex(unsigned n, MutexPool& p) {
  (void)create_list_using_pool(n, p);
}
static void test_global_mutex() {
  cout << "Global mutexed pool:\n";
  struct rusage start, finish;
  get_usage(start);
  {
    size_t total_capacity = NUM_THREADS * N * sizeof(Node);
    MutexPool pool(total_capacity, sizeof(Node));

    vector<thread> threads;
    for (unsigned i = 0; i < NUM_THREADS; i++)
      threads.emplace_back(thread_global_mutex, N, std::ref(pool));
    for (auto& t : threads) t.join();
  }
  get_usage(finish);
  print_stats(start, finish, NUM_THREADS * N);
}

#elif defined(USE_LOCKFREE_POOL)
static void thread_global_lockfree(unsigned n, LockFreePool& p) {
  (void)create_list_using_pool(n, p);
}
static void test_global_lockfree() {
  cout << "Global lock-free pool:\n";
  struct rusage start, finish;
  get_usage(start);
  {
    size_t total_capacity = NUM_THREADS * N * sizeof(Node);
    LockFreePool pool(total_capacity, sizeof(Node));

    vector<thread> threads;
    for (unsigned i = 0; i < NUM_THREADS; i++)
      threads.emplace_back(thread_global_lockfree, N, std::ref(pool));
    for (auto& t : threads) t.join();
  }
  get_usage(finish);
  print_stats(start, finish, NUM_THREADS * N);
}

#elif defined(USE_THREAD_LOCAL_POOL)
static void thread_local_pool(unsigned n) {
  Pool p(n * sizeof(Node), sizeof(Node));
  (void)create_list_using_pool(n, p);
}
static void test_local_pools() {
  cout << "Thread-local pools:\n";
  struct rusage start, finish;
  get_usage(start);

  vector<thread> threads;
  for (unsigned i = 0; i < NUM_THREADS; i++)
    threads.emplace_back(thread_local_pool, N);
  for (auto& t : threads) t.join();

  get_usage(finish);
  print_stats(start, finish, NUM_THREADS * N);
}

#else
static void thread_std(unsigned n) {
  delete_list(create_list(n));
}
static void test_std() {
  cout << "Standard Allocator:\n";
  struct rusage start, finish;
  get_usage(start);
  {
    vector<thread> threads;
    for (unsigned i = 0; i < NUM_THREADS; i++)
      threads.emplace_back(thread_std, N);
    for (auto& t : threads) t.join();
  }
  get_usage(finish);
  print_stats(start, finish, NUM_THREADS * N);
}
#endif

int main() {
#if defined(USE_MUTEX_POOL)
  test_global_mutex();
#elif defined(USE_LOCKFREE_POOL)
  test_global_lockfree();
#elif defined(USE_THREAD_LOCAL_POOL)
  test_local_pools();
#else
  test_std();
#endif
  return EXIT_SUCCESS;
}
