#include "cmem/freelist.hpp"
#include <climits>

namespace cmem {

// ═══════════════════════════════════════════════════════════════
//  ImplicitFreeList
// ═══════════════════════════════════════════════════════════════

BlockHeader* ImplicitFreeList::find_fit(BlockHeader* heap_start,
                                         BlockHeader* heap_end,
                                         size_t       block_needed) {
    BlockHeader* cur = heap_start->next();           // skip prologue
    while (cur != heap_end && cur->size() > 0) {
        if (cur->free() && cur->size() >= block_needed)
            return cur;
        cur = cur->next();
    }
    return nullptr;
}

// ═══════════════════════════════════════════════════════════════
//  ExplicitFreeList – Best Fit
// ═══════════════════════════════════════════════════════════════

BlockHeader* ExplicitFreeList::find_fit(BlockHeader* /*heap_start*/,
                                         BlockHeader* /*heap_end*/,
                                         size_t       block_needed) {
    // First Fit — size classes already group similar sizes
    for (BlockHeader* cur = head_; cur; cur = cur->link()->next) {
        if (cur->size() >= block_needed)
            return cur;
    }
    return nullptr;
}

void ExplicitFreeList::insert(BlockHeader* block) {
    block->link()->next = head_;
    block->link()->prev = nullptr;
    if (head_) head_->link()->prev = block;
    head_ = block;
}

void ExplicitFreeList::remove(BlockHeader* block) {
    if (block->link()->prev)
        block->link()->prev->link()->next = block->link()->next;
    else
        head_ = block->link()->next;

    if (block->link()->next)
        block->link()->next->link()->prev = block->link()->prev;
}

// ═══════════════════════════════════════════════════════════════
//  SegregatedFreeLists – 16 power-of-2 size classes
// ═══════════════════════════════════════════════════════════════

int SegregatedFreeLists::class_index(size_t block_sz) {
    if (block_sz <= MIN_BLOCK) return 0;
    int bits = static_cast<int>(8 * sizeof(size_t) - __builtin_clzll(block_sz - 1));
    int cls = bits - 5;  // log₂(MIN_BLOCK) = 5  (MIN_BLOCK = 32)
    return (cls < NUM_CLASSES) ? cls : (NUM_CLASSES - 1);
}

BlockHeader* SegregatedFreeLists::find_fit(BlockHeader* heap_start,
                                            BlockHeader* heap_end,
                                            size_t       block_needed) {
    int start = class_index(block_needed);
    for (int i = start; i < NUM_CLASSES; ++i) {
        BlockHeader* b = lists_[i].find_fit(heap_start, heap_end, block_needed);
        if (b) return b;
    }
    return nullptr;
}

void SegregatedFreeLists::insert(BlockHeader* block) {
    lists_[class_index(block->size())].insert(block);
}

void SegregatedFreeLists::remove(BlockHeader* block) {
    lists_[class_index(block->size())].remove(block);
}

} // namespace cmem
