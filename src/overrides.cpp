// Compile & link this file to replace the global operator new/delete
// with the cmem allocator.  All C++ allocations (std::string, std::vector,
// etc.) will transparently use the custom allocator.
//
//   g++ -c overrides.cpp -I../include
//   g++ ... overrides.o -lcmem

#include "cmem/allocator.hpp"
#include <new>

void* operator new(size_t size) {
    auto* p = cmem::Allocator::instance().malloc(size);
    if (!p) throw std::bad_alloc();
    return p;
}

void* operator new[](size_t size) {
    return operator new(size);
}

void operator delete(void* ptr) noexcept {
    cmem::Allocator::instance().free(ptr);
}

void operator delete[](void* ptr) noexcept {
    cmem::Allocator::instance().free(ptr);
}

void operator delete(void* ptr, size_t) noexcept {
    cmem::Allocator::instance().free(ptr);
}

void operator delete[](void* ptr, size_t) noexcept {
    cmem::Allocator::instance().free(ptr);
}
