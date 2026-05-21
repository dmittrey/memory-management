#pragma once

#include <cstddef>

void init_parallel_copy_pool(std::size_t num_workers);
void shutdown_parallel_copy_pool();
void* parallel_memcpy(void* dst, const void* src, std::size_t size);
