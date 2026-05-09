#include "cmem/common.hpp"
#include "cmem/allocator.hpp"
#include <cstdio>
#include <cassert>
#include <cstring>

using namespace cmem;

static void test_basic() {
    printf("\n-- Allocator (SegregatedFreeLists)\n");

    auto& A = Allocator::instance();
    auto s = A.stats();
    printf("  initial: total=%zu  used=%zu  free=%zu  blocks=%d\n",
           s.total_bytes, s.used_bytes, s.free_bytes, s.block_count);
    assert(s.free_bytes > 0 && s.used_bytes == 0);

    // malloc / free
    void* p1 = A.malloc(100);
    assert(p1);
    memset(p1, 0xAB, 100);

    void* p2 = A.malloc(500);
    assert(p2);
    memset(p2, 0xCD, 500);

    s = A.stats();
    printf("  after 2 mallocs: used=%zu  blocks=%d\n",
           s.used_bytes, s.block_count);
    assert(s.used_bytes > 0);
    assert(static_cast<char*>(p1)[0] == static_cast<char>(0xAB));
    assert(static_cast<char*>(p2)[0] == static_cast<char>(0xCD));

    A.free(p1);
    A.free(p2);
    s = A.stats();
    printf("  after 2 frees:  used=%zu  blocks=%d\n",
           s.used_bytes, s.block_count);
    assert(s.used_bytes == 0);

    // calloc (zero-initialised)
    int* arr = static_cast<int*>(A.calloc(100, sizeof(int)));
    assert(arr);
    for (int i = 0; i < 100; ++i) assert(arr[i] == 0);
    arr[50] = 42;
    assert(arr[50] == 42);
    A.free(arr);
    printf("  calloc: ok\n");

    // realloc
    void* rp = A.malloc(64);
    assert(rp);
    memset(rp, 0xFF, 64);

    rp = A.realloc(rp, 128);       // grow
    assert(rp);
    assert(static_cast<char*>(rp)[0] == static_cast<char>(0xFF));
    static_cast<char*>(rp)[127] = 0xEE;
    assert(static_cast<char*>(rp)[127] == static_cast<char>(0xEE));

    rp = A.realloc(rp, 32);        // fits in place (no move)
    assert(rp);

    A.free(rp);

    rp = A.realloc(nullptr, 64);   // acts as malloc
    assert(rp);
    memset(rp, 0xAA, 64);
    void* tmp = A.realloc(rp, 0);  // acts as free
    assert(tmp == nullptr);
    rp = nullptr;
    printf("  realloc: ok\n");

    // operator new / delete (drop-in)
    printf("  operator new/delete: ");
    int* ip = new int(42);
    assert(*ip == 42);
    delete ip;

    int* arr2 = new int[100];
    for (int i = 0; i < 100; ++i) arr2[i] = i;
    assert(arr2[50] == 50);
    delete[] arr2;
    printf("ok\n");

    // final state
    s = A.stats();
    printf("  final: used=%zu  blocks=%d\n", s.used_bytes, s.block_count);
    assert(s.used_bytes == 0);
    printf("-- Allocator: PASSED\n");
}

int main() {
    printf("=== Phase 5: Allocator (strategy = ");
#if defined(CMEM_SEGREGATED)
    printf("SEGREGATED");
#elif defined(CMEM_EXPLICIT)
    printf("EXPLICIT");
#else
    printf("IMPLICIT");
#endif
    printf(") ===\n");

    test_basic();

    printf("\n=== Phase 5: ALL TESTS PASSED ===\n");
    return 0;
}
