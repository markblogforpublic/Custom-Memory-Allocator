#include "cmem/allocator.hpp"
#include "cmem/heap.hpp"
#include "cmem/freelist.hpp"
#include <cstring>
#include <cstdlib>       // std::malloc, std::free
#include <new>           // placement new

namespace cmem {

// ═══════════════════════════════════════════════════════════════
//  Singleton — uses std::malloc (NOT operator new) to avoid
//  circular dependency when global operator new is overridden.
// ═══════════════════════════════════════════════════════════════

Allocator& Allocator::instance() {
    static Allocator alloc;
    return alloc;
}

Allocator::Allocator() {
#if defined(CMEM_SEGREGATED)
    void* fm = std::malloc(sizeof(SegregatedFreeLists));
    fl_ = new (fm) SegregatedFreeLists();
#elif defined(CMEM_EXPLICIT)
    void* fm = std::malloc(sizeof(ExplicitFreeList));
    fl_ = new (fm) ExplicitFreeList();
#else
    void* fm = std::malloc(sizeof(ImplicitFreeList));
    fl_ = new (fm) ImplicitFreeList();
#endif

    void* hm = std::malloc(sizeof(Heap));
    heap_ = new (hm) Heap(fl_, false);       // Heap does NOT own fl_
}

Allocator::~Allocator() {
    heap_->~Heap();
    std::free(heap_);
    fl_->~FreeList();
    std::free(fl_);
}

// ═══════════════════════════════════════════════════════════════
//  Public API
// ═══════════════════════════════════════════════════════════════

void* Allocator::malloc(size_t size) {
    return heap_->allocate(size);
}

void Allocator::free(void* ptr) noexcept {
    heap_->deallocate(ptr);
}

void* Allocator::realloc(void* ptr, size_t new_size) {
    if (!ptr) return heap_->allocate(new_size);
    if (new_size == 0) {
        heap_->deallocate(ptr);
        return nullptr;
    }

    BlockHeader* block = block_from_ptr(ptr);
    size_t old_payload = block->size() - sizeof(BlockHeader) - sizeof(size_t);

    if (old_payload >= new_size) return ptr;

    void* new_ptr = heap_->allocate(new_size);
    if (!new_ptr) return nullptr;

    std::memcpy(new_ptr, ptr, old_payload);
    heap_->deallocate(ptr);
    return new_ptr;
}

void* Allocator::calloc(size_t num, size_t size) {
    size_t total = num * size;
    void* ptr = heap_->allocate(total);
    if (ptr) std::memset(ptr, 0, total);
    return ptr;
}

Allocator::Stats Allocator::stats() const {
    Stats s;
    s.total_bytes = heap_->total_bytes();
    s.used_bytes  = heap_->used_bytes();
    s.free_bytes  = heap_->free_bytes();
    s.block_count = heap_->block_count();
    return s;
}

} // namespace cmem
