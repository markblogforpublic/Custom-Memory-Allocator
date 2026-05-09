#include "cmem/heap.hpp"
#include "cmem/platform.hpp"
#include <cstring>
#include <cstdlib>       // std::malloc, std::free
#include <new>

namespace cmem {

static constexpr size_t PROLOGUE_SZ = 16;

// ── helpers ──────────────────────────────────────────────────────

static size_t block_needed(size_t user_size) {
    if (user_size == 0) user_size = 1;
    size_t payload = align_up(user_size, ALIGNMENT);
    if (payload < sizeof(BlockHeader::Link))
        payload = sizeof(BlockHeader::Link);
    size_t total = sizeof(BlockHeader) + payload + sizeof(size_t);
    return align_up(total, ALIGNMENT);
}

// ── ctor / dtor ──────────────────────────────────────────────────

Heap::Heap(FreeList* fl, bool own_fl, size_t chunk_sz)
    : chunk_list_(nullptr), free_list_(fl), own_fl_(own_fl) {
    (void)chunk_sz;
    add_chunk(1);
}

Heap::~Heap() {
    auto* cur = chunk_list_;
    while (cur) {
        auto* next = cur->next;
        os_release(cur->base, cur->size);
        std::free(cur);
        cur = next;
    }
    if (own_fl_) delete free_list_;
}

// ── chunk management ─────────────────────────────────────────────

void Heap::add_chunk(size_t min_block_sz) {
    size_t sz = CHUNK_SIZE;
    while (sz < min_block_sz + PROLOGUE_SZ + sizeof(size_t))
        sz *= 2;

    void* raw = os_reserve(sz);
    if (!raw) throw std::bad_alloc();

    BlockHeader* prologue = static_cast<BlockHeader*>(raw);
    prologue->init(PROLOGUE_SZ, ALLOC_BIT);

    BlockHeader* first  = prologue->next();
    size_t free_sz      = sz - PROLOGUE_SZ - sizeof(size_t);
    first->init(free_sz, FREE_BIT);
    free_list_->insert(first);

    BlockHeader* epilogue = reinterpret_cast<BlockHeader*>(
        static_cast<char*>(raw) + sz - sizeof(size_t));
    epilogue->size_and_flag = pack_size(0, ALLOC_BIT);

    auto* node = static_cast<ChunkNode*>(std::malloc(sizeof(ChunkNode)));
    node->next  = chunk_list_;
    node->start = prologue;
    node->end   = epilogue;
    node->size  = sz;
    node->base  = raw;
    chunk_list_ = node;
}

// ── allocation ────────────────────────────────────────────────────

void* Heap::allocate(size_t size) {
    size_t need = block_needed(size);
    BlockHeader* block;

    for (int attempt = 0; attempt <= 1; ++attempt) {
        block = free_list_->find_fit(chunk_list_->start,
                                      chunk_list_->end, need);
        if (block) break;
        if (attempt == 0) add_chunk(need);
        else              return nullptr;
    }

    free_list_->remove(block);
    split(block, need);
    return block->payload();
}

void Heap::split(BlockHeader* block, size_t block_needed) {
    size_t remain = block->size() - block_needed;
    if (remain >= MIN_BLOCK) {
        block->init(block_needed, ALLOC_BIT);
        BlockHeader* new_block = block->next();
        new_block->init(remain, FREE_BIT);
        free_list_->insert(new_block);
    } else {
        block->set_allocated();
    }
}

// ── deallocation ──────────────────────────────────────────────────

void Heap::deallocate(void* ptr) {
    if (!ptr) return;
    BlockHeader* block = block_from_ptr(ptr);
    block->init(block->size(), FREE_BIT);
    block = coalesce(block);
    free_list_->insert(block);
}

BlockHeader* Heap::coalesce(BlockHeader* block) {
    for (;;) {
        BlockHeader* nb = block->next();
        if (nb->size() == 0 || !nb->free()) break;
        free_list_->remove(nb);
        block->init(block->size() + nb->size(), FREE_BIT);
    }

    BlockHeader* pb = block->prev();
    if (pb->alloc()) return block;   // prologue is always allocated

    free_list_->remove(pb);
    pb->init(pb->size() + block->size(), FREE_BIT);
    block = pb;

    for (;;) {
        BlockHeader* nb = block->next();
        if (nb->size() == 0 || !nb->free()) break;
        free_list_->remove(nb);
        block->init(block->size() + nb->size(), FREE_BIT);
    }

    return block;
}

// ── introspection ────────────────────────────────────────────────

size_t Heap::total_bytes() const {
    size_t t = 0;
    for (auto* cur = chunk_list_; cur; cur = cur->next)
        t += cur->size;
    return t;
}

size_t Heap::free_bytes() const {
    size_t sz = 0;
    for (auto* cur = chunk_list_; cur; cur = cur->next) {
        auto* b = cur->start->next();
        while (b != cur->end && b->size() > 0) {
            if (b->free()) sz += b->size();
            b = b->next();
        }
    }
    return sz;
}

size_t Heap::used_bytes() const {
    size_t used = 0;
    for (auto* cur = chunk_list_; cur; cur = cur->next) {
        auto* b = cur->start->next();
        while (b != cur->end && b->size() > 0) {
            if (b->alloc()) used += b->size();
            b = b->next();
        }
    }
    return used;
}

int Heap::block_count() const {
    int n = 0;
    for (auto* cur = chunk_list_; cur; cur = cur->next) {
        auto* b = cur->start->next();
        while (b != cur->end && b->size() > 0) {
            ++n;
            b = b->next();
        }
    }
    return n;
}

} // namespace cmem
