cmake_minimum_required(VERSION 3.16)
project(cmem VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS_RELEASE "-O2")

# ── strategy selection ──────────────────────────────────────
set(CMEM_STRATEGY "SEGREGATED" CACHE STRING
    "Allocation strategy: IMPLICIT, EXPLICIT, SEGREGATED")
if(CMEM_STRATEGY STREQUAL "IMPLICIT")
    add_compile_definitions(CMEM_IMPLICIT)
elseif(CMEM_STRATEGY STREQUAL "EXPLICIT")
    add_compile_definitions(CMEM_EXPLICIT)
elseif(CMEM_STRATEGY STREQUAL "SEGREGATED")
    add_compile_definitions(CMEM_SEGREGATED)
else()
    message(FATAL_ERROR "Unknown CMEM_STRATEGY: ${CMEM_STRATEGY}")
endif()

# ── library ──────────────────────────────────────────────────
add_library(cmem
    src/platform.cpp
    src/heap.cpp
    src/freelist.cpp
    src/allocator.cpp
)

target_include_directories(cmem PUBLIC include)

# ── demo executable ─────────────────────────────────────────
add_executable(cmem_demo src/main.cpp)
target_link_libraries(cmem_demo cmem)

# ── benchmark ────────────────────────────────────────────────
add_executable(cmem_bench benchmarks/benchmark.cpp)
target_link_libraries(cmem_bench cmem)
