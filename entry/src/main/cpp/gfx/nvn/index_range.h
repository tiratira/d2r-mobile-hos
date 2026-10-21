#pragma once
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>

// Constant-size loads allow the compiler to vectorize this reduction. Calling
// variable-size memcpy for each index was a substantial part of CPU replay.
template <typename T> inline uint32_t nvnIndexLimit(const uint8_t *data, size_t count) {
    uint32_t limit = 0;
    for (size_t i = 0; i < count; ++i) {
        T value;
        memcpy(&value, data + i * sizeof(T), sizeof(T));
        if constexpr (sizeof(T) > 1) {
            if (value == T(-1))
                continue;
        }
        limit = std::max(limit, uint32_t(value) + 1);
    }
    return limit;
}

inline uint32_t nvnIndexLimit(const uint8_t *data, size_t count, int type) {
    switch (type) {
    case 2: return nvnIndexLimit<uint32_t>(data, count);
    case 1: return nvnIndexLimit<uint16_t>(data, count);
    default: return nvnIndexLimit<uint8_t>(data, count);
    }
}
