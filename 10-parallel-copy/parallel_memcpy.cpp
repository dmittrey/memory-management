#include "parallel_memcpy.h"

#include "pool.h"

#include <cstring>
#include <memory>

namespace {

std::unique_ptr<ThreadPool> g_pool;

}  // namespace

void init_parallel_copy_pool(std::size_t num_workers) {
  g_pool = std::make_unique<ThreadPool>(num_workers);
}

void shutdown_parallel_copy_pool() {
  g_pool.reset();
}

void* parallel_memcpy(void* dst, const void* src, std::size_t size) {
  if (size == 0) {
    return dst;
  }

  auto* dst_bytes = static_cast<unsigned char*>(dst);
  const auto* src_bytes = static_cast<const unsigned char*>(src);

  if (g_pool == nullptr || g_pool->num_workers() == 0) {
    std::memcpy(dst_bytes, src_bytes, size);
    return dst;
  }

  const std::size_t num_workers = g_pool->num_workers();
  const std::size_t per_worker_chunk = size / (num_workers + 1);
  const std::size_t main_offset = num_workers * per_worker_chunk;
  const std::size_t main_chunk = size - main_offset;

  g_pool->begin_task(ThreadPool::Task{
      src_bytes,
      dst_bytes,
      per_worker_chunk,
  });

  std::memcpy(dst_bytes + main_offset, src_bytes + main_offset, main_chunk);
  g_pool->wait_for_workers();

  return dst;
}
