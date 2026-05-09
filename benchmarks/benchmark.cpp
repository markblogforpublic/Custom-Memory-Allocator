#include "cmem/allocator.hpp"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <chrono>
#include <vector>

using namespace cmem;
using Clock = std::chrono::high_resolution_clock;

static int rand_int(int lo, int hi) {
    return lo + (std::rand() % (hi - lo + 1));
}

struct Result {
    const char* name;
    double      elapsed;
    long long   ops;
};

static Result run_bench(const char* name,
                         void* (*alloc_fn)(size_t),
                         void  (*free_fn)(void*),
                         int count, int min_sz, int max_sz) {
    std::vector<void*> ptrs;
    ptrs.reserve(count);

    auto start = Clock::now();
    for (int i = 0; i < count; ++i) {
        int sz = rand_int(min_sz, max_sz);
        ptrs.push_back(alloc_fn(sz));
        if (!ptrs[i]) { fprintf(stderr, "FAIL at %d\n", i); std::exit(1); }
        memset(ptrs[i], i & 0xFF, 8);
    }
    for (int i = 0; i < count; ++i)
        free_fn(ptrs[i]);
    auto end = Clock::now();
    return {name, std::chrono::duration<double>(end - start).count(), count};
}

static void print_result(const Result& r) {
    printf("  %-30s  %8lld  ops  %7.3f  s  %10.0f  ops/s\n",
           r.name, r.ops, r.elapsed, r.ops / r.elapsed);
}

static auto& g_alloc = Allocator::instance();

extern "C" void* cmem_malloc(size_t s) { return g_alloc.malloc(s); }
extern "C" void  cmem_free(void* p)    { g_alloc.free(p); }

int main() {
    std::srand(42);

    printf("+----------------------------------------------------------+\n");
    printf("| cmem Allocator  vs  system malloc/free                    |\n");
    printf("+----------------------------------------------------------+\n");
    printf("| %-30s  %8s  %7s  %11s |\n", "Benchmark", "Count", "Time", "Throughput");
    printf("+----------------------------------------------------------+\n");

    print_result(run_bench("cmem  small (8-64)",    cmem_malloc, cmem_free, 100000, 8,   64));
    print_result(run_bench("libc  small (8-64)",    std::malloc, std::free,  100000, 8,   64));
    print_result(run_bench("cmem  medium (128-512)", cmem_malloc, cmem_free, 50000, 128, 512));
    print_result(run_bench("libc  medium (128-512)", std::malloc, std::free, 50000, 128, 512));
    print_result(run_bench("cmem  large (1-8K)",    cmem_malloc, cmem_free, 5000,  1024, 8192));
    print_result(run_bench("libc  large (1-8K)",    std::malloc, std::free,  5000,  1024, 8192));

    printf("+----------------------------------------------------------+\n");
    return 0;
}
