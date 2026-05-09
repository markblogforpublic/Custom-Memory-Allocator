#pragma once
#include "cmem/block.hpp"
#include "cmem/common.hpp"
#include <cstddef>
#include <climits>

namespace cmem {

// ── FreeList base ──────────────────────────────────────────────

class FreeList {
public:
    virtual ~FreeList() = default;

    virtual BlockHeader* find_fit(BlockHeader* heap_start,
                                  BlockHeader* heap_end,
                                  size_t       block_needed) = 0;

    virtual void insert(BlockHeader* block) = 0;
    virtual void remove(BlockHeader* block) = 0;
};

// ── ImplicitFreeList (Phase 2 equivalence) ────────────────────
//     No explicit pointers — walks every block via size field.

class ImplicitFreeList final : public FreeList {
public:
    BlockHeader* find_fit(BlockHeader* heap_start,
                          BlockHeader* heap_end,
                          size_t       block_needed) override;

    void insert(BlockHeader* block) override { (void)block; }
    void remove(BlockHeader* block) override { (void)block; }
};

// ── ExplicitFreeList (doubly-linked list of free blocks) ──────
//     First-Fit within the list (used per size-class by Segregated).

class ExplicitFreeList final : public FreeList {
    BlockHeader* head_;

public:
    ExplicitFreeList() : head_(nullptr) {}

    BlockHeader* find_fit(BlockHeader* heap_start,
                          BlockHeader* heap_end,
                          size_t       block_needed) override;

    void insert(BlockHeader* block) override;
    void remove(BlockHeader* block) override;

    BlockHeader* head() const { return head_; }
};

// ── SegregatedFreeLists (16 power-of-2 size classes) ──────────
//     Each class uses Best Fit internally.

class SegregatedFreeLists final : public FreeList {
    static constexpr int NUM_CLASSES = 16;
    ExplicitFreeList lists_[NUM_CLASSES];

    static int class_index(size_t block_sz);

public:
    BlockHeader* find_fit(BlockHeader* heap_start,
                          BlockHeader* heap_end,
                          size_t       block_needed) override;

    void insert(BlockHeader* block) override;
    void remove(BlockHeader* block) override;
};

} // namespace cmem
