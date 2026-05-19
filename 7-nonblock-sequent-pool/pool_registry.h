#pragma once

#include <cstddef>

namespace pool_registry {

void register_pool(const char* name, const void* begin, const void* end);
void unregister_pool(const void* begin);
void install_segv_handler();

}  // namespace pool_registry
