#pragma once
#include <cstddef>
#include <cstdint>

namespace cmem {

constexpr size_t ALIGNMENT   = 8;
constexpr size_t MIN_BLOCK   = 32;  // header(8) + prev(8) + next(8) + footer(8)
constexpr size_t CHUNK_SIZE  = 64 * 1024;  // 64 KB
constexpr size_t MAX_CLASSES = 32;

constexpr size_t ALLOC_BIT = 1;
constexpr size_t FREE_BIT  = 0;

inline size_t align_up(size_t n, size_t a = ALIGNMENT) {
    return (n + a - 1) & ~(a - 1);
}

inline size_t align_down(size_t n, size_t a = ALIGNMENT) {
    return n & ~(a - 1);
}

inline size_t pack_size(size_t sz, size_t flag) {
    return sz | flag;
}

inline size_t block_size(size_t packed) {
    return packed & ~ALLOC_BIT;
}

inline bool is_alloc(size_t packed) {
    return (packed & ALLOC_BIT) != 0;
}

} // namespace cmem
