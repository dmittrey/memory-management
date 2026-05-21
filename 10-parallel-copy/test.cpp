#include "parallel_memcpy.h"

#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <vector>

namespace {

constexpr std::size_t kCopySize = 256ULL * 1024ULL * 1024ULL;
constexpr std::size_t kAlignment = 4096;
constexpr int kWarmupRuns = 2;
constexpr int kMeasuredRuns = 10;

double median(std::vector<double> values) {
  std::sort(values.begin(), values.end());
  const std::size_t mid = values.size() / 2;
  if (values.size() % 2 == 0) {
    return (values[mid - 1] + values[mid]) / 2.0;
  }
  return values[mid];
}

void fill_pattern(unsigned char* buffer, std::size_t size) {
  for (std::size_t i = 0; i < size; ++i) {
    buffer[i] = static_cast<unsigned char>((i * 2654435761u) >> 24);
  }
}

}  // namespace

int main() {
  unsigned char* src = nullptr;
  unsigned char* dst = nullptr;

  if (posix_memalign(reinterpret_cast<void**>(&src), kAlignment, kCopySize) != 0 ||
      posix_memalign(reinterpret_cast<void**>(&dst), kAlignment, kCopySize) != 0) {
        throw std::runtime_exception("Failed to allocate buffers");
  }

  fill_pattern(src, kCopySize);
  std::memset(dst, 0, kCopySize);

  std::memcpy(dst, src, kCopySize);
  std::memset(dst, 0, kCopySize);

  std::cout << std::fixed << std::setprecision(3);
  std::cout << "Copy size: " << (static_cast<double>(kCopySize) / (1024.0 * 1024.0))
            << " MiB\n\n";
  std::cout << "Pool size  Time (ms)  Throughput (GB/s)\n";
  std::cout.flush();

  for (std::size_t pool_size = 0; pool_size <= 8; ++pool_size) {
    init_parallel_copy_pool(pool_size);

    parallel_memcpy(dst, src, kCopySize);
    if (std::memcmp(dst, src, kCopySize) != 0) {
      std::cerr << "Correctness check failed for pool size " << pool_size << '\n';
      shutdown_parallel_copy_pool();
      std::free(src);
      std::free(dst);
      return EXIT_FAILURE;
    }

    for (int i = 0; i < kWarmupRuns; ++i) {
      parallel_memcpy(dst, src, kCopySize);
    }

    std::vector<double> times_ms;
    times_ms.reserve(kMeasuredRuns);
    for (int i = 0; i < kMeasuredRuns; ++i) {
      const auto start = std::chrono::steady_clock::now();
      parallel_memcpy(dst, src, kCopySize);
      const auto finish = std::chrono::steady_clock::now();

      const double elapsed_ms =
          std::chrono::duration<double, std::milli>(finish - start).count();
      times_ms.push_back(elapsed_ms);
    }

    const double median_ms = median(times_ms);
    const double throughput_gbps =
        (static_cast<double>(kCopySize) / (1024.0 * 1024.0 * 1024.0)) /
        (median_ms / 1000.0);

    std::cout << std::setw(9) << pool_size << std::setw(11) << median_ms
              << std::setw(18) << throughput_gbps << '\n';
    std::cout.flush();

    shutdown_parallel_copy_pool();
  }

  std::free(src);
  std::free(dst);
  return EXIT_SUCCESS;
}
