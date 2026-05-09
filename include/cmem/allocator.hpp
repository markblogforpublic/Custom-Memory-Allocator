#pragma once
#include <cstddef>
#include <cstdint>

namespace cmem {

class Heap;
class FreeList;

class Allocator {
    Heap*    heap_;
    FreeList* fl_;

public:
    Allocator();
    ~Allocator();

    Allocator(const Allocator&) = delete;
    Allocator& operator=(const Allocator&) = delete;

    void* malloc(size_t size);
    void  free(void* ptr) noexcept;
    void* realloc(void* ptr, size_t new_size);
    void* calloc(size_t num, size_t size);

    struct Stats {
        size_t total_bytes;
        size_t used_bytes;
        size_t free_bytes;
        int    block_count;
    };

    Stats stats() const;

    static Allocator& instance();
};

} // namespace cmem
