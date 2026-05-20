#pragma once

namespace pool_registry {

int register_pool(const void* guard_start, const void* guard_end);
void unregister_pool(int id);

}  // namespace pool_registry
