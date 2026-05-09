#pragma once
#include <cstddef>
#include "common.hpp"

namespace cmem {

struct BlockHeader {
    size_t size_and_flag;

    size_t size()  const { return block_size(size_and_flag); }
    bool   free()  const { return !is_alloc(size_and_flag); }
    bool   alloc() const { return  is_alloc(size_and_flag); }

    void set_free()       { size_and_flag = pack_size(size(), 0); }
    void set_allocated()  { size_and_flag = pack_size(size(), ALLOC_BIT); }

    void init(size_t sz, size_t flag) {
        size_and_flag = pack_size(sz, flag);
        footer() = size_and_flag;
    }

    // ── address arithmetic ──────────────────────────────────

    BlockHeader* next() const {
        return reinterpret_cast<BlockHeader*>(
            reinterpret_cast<char*>(const_cast<BlockHeader*>(this)) + size());
    }

    BlockHeader* prev() const {
        size_t* prev_footer = reinterpret_cast<size_t*>(
            reinterpret_cast<char*>(const_cast<BlockHeader*>(this)) - sizeof(size_t));
        size_t prev_sz = block_size(*prev_footer);
        return reinterpret_cast<BlockHeader*>(
            reinterpret_cast<char*>(const_cast<BlockHeader*>(this)) - prev_sz);
    }

    // ── footer ──────────────────────────────────────────────

    size_t& footer() {
        return *reinterpret_cast<size_t*>(
            reinterpret_cast<char*>(this) + size() - sizeof(size_t));
    }

    const size_t& footer() const {
        return *reinterpret_cast<const size_t*>(
            reinterpret_cast<const char*>(this) + size() - sizeof(size_t));
    }

    // ── payload (user data starts here) ─────────────────────

    void*       payload()       { return static_cast<void*>(this + 1); }
    const void* payload() const { return static_cast<const void*>(this + 1); }

    // ── free-list pointers stored in payload (when free) ────

    struct Link {
        BlockHeader* prev;
        BlockHeader* next;
    };

    Link* link() {
        return static_cast<Link*>(payload());
    }

    const Link* link() const {
        return static_cast<const Link*>(payload());
    }
};

// Get block header from user pointer returned by malloc()
inline BlockHeader* block_from_ptr(void* ptr) {
    return static_cast<BlockHeader*>(ptr) - 1;
}

inline const BlockHeader* block_from_ptr(const void* ptr) {
    return static_cast<const BlockHeader*>(ptr) - 1;
}

} // namespace cmem
