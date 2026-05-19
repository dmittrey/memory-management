#pragma once

#include <cstdint>
#include <optional>

std::optional<uint8_t> safe_read_uint8(const uint8_t* p);
