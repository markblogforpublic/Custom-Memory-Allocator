#pragma once
#include "cmem/block.hpp"
#include "cmem/common.hpp"
#include "cmem/freelist.hpp"
#include <cstddef>

namespace cmem {

class Heap {
    struct ChunkNode {
        ChunkNode*   next;
        BlockHeader* start;       // prologue
        BlockHeader* end;         // epilogue
        size_t       size;        // total chunk size
        void*        base;        // os_reserve pointer
    };

    ChunkNode* chunk_list_;
    FreeList*  free_list_;
    bool       own_fl_;

public:
    explicit Heap(FreeList* fl, bool own_fl = true,
                  size_t chunk_sz = CHUNK_SIZE);

    ~Heap();

    Heap(const Heap&) = delete;
    Heap& operator=(const Heap&) = delete;

    void* allocate(size_t size);
    void  deallocate(void* ptr);

    size_t total_bytes() const;
    size_t free_bytes() const;
    size_t used_bytes() const;
    int    block_count() const;

private:
    void         add_chunk(size_t min_block_sz);
    void         split(BlockHeader* block, size_t block_needed);
    BlockHeader* coalesce(BlockHeader* block);
};

} // namespace cmem
