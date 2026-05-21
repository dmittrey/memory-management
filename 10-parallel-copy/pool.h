#pragma once

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <thread>
#include <vector>

class ThreadPool {
 public:
  struct Task {
    const unsigned char* src = nullptr;
    unsigned char* dst = nullptr;
    std::size_t per_worker_chunk = 0;
  };

  explicit ThreadPool(std::size_t num_workers) {
    if (num_workers == 0) {
      throw std::invalid_argument("workers should be more than zero");
    }

    workers_.reserve(num_workers);
    for (std::size_t i = 0; i < num_workers; ++i) {
      workers_.emplace_back([this, i] { worker_loop(i); });
    }

    while (ready_count_.load() < num_workers) {
      spin_wait();
    }
  }

  ~ThreadPool() {
    stop_.store(true);
    epoch_.fetch_add(1);
    for (auto& worker : workers_) {
      worker.join();
    }
  }

  ThreadPool(const ThreadPool&) = delete;
  ThreadPool& operator=(const ThreadPool&) = delete;

  std::size_t num_workers() const { return workers_.size(); }

  void begin_task(const Task& task) {
    task_ = task;
    done_.store(0);
    epoch_.fetch_add(1);
  }

  void wait_for_workers() {
    const std::size_t expected = workers_.size();
    while (done_.load(std::memory_order_acquire) != expected) {
      spin_wait();
    }
  }

 private:
  static void spin_wait() {
#if defined(__x86_64__) || defined(__i386__)
    __builtin_ia32_pause();
#elif defined(__aarch64__) || defined(__arm__)
    __asm__ volatile("yield");
#else
    __asm__ volatile("");
#endif
  }

  void worker_loop(std::size_t worker_id) {
    std::uint64_t seen_epoch = epoch_.load();
    ready_count_.fetch_add(1);

    while (true) {
      while (epoch_.load() == seen_epoch &&
             !stop_.load()) {
        spin_wait();
      }

      if (stop_.load()) {
        return;
      }

      seen_epoch = epoch_.load();

      const Task task = task_;
      const std::size_t offset = worker_id * task.per_worker_chunk;
      std::memcpy(task.dst + offset, task.src + offset, task.per_worker_chunk);

      done_.fetch_add(1);
    }
  }

  std::vector<std::thread> workers_;
  Task task_{};
  std::atomic<std::uint64_t> epoch_{0};
  std::atomic<std::size_t> done_{0};
  std::atomic<bool> stop_{false};
  std::atomic<std::size_t> ready_count_{0};
};
