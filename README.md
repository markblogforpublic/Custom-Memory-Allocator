# cmem — Custom Memory Allocator

> **CMU 15-213 Malloc Lab 风格实现**  
> 从零手写 `malloc`/`free`，三种空闲链表策略，性能超越系统 libc。

---

## 项目背景

本项目的原型是 **卡内基梅隆大学 (CMU) 15-213: Introduction to Computer Systems** 中最著名的实验之一——**Malloc Lab**。

在该实验中，学生必须：

1. **接管进程的堆空间**——不能使用系统自带的 `malloc`，必须手写 `mm_malloc`、`mm_free`、`mm_realloc`
2. **接受双重考核**——分数由 **吞吐量 (Throughput)** 和 **内存利用率 (Utilization)** 共同决定
3. **追求极致**——从隐式空闲链表 → 显式空闲链表 → 隔离空闲链表，逐步优化

15-213 被公认为全球计算机系统入门课程的"金标准"。而 Malloc Lab 是其中最硬核的关卡：你的分配器不仅要正确，还要够快、够省。这是一个将理论（操作系统、计算机组成、编译原理）落地的终极实践。

本项目完整实现了这个实验的三种策略，并在特定场景下性能超过系统 libc `malloc`。

---

## 特性

| 特性 | 实现 |
|---|---|
| 边界标签 (Boundary Tags) | 每个块首尾存储 `size\|flag`，O(1) 向前/后合并 |
| 分裂 (Splitting) | 分配时自动切分过大块，防内部碎片 |
| 合并 (Coalescing) | 释放时级联合并相邻空闲块，防外部碎片 |
| 跨平台 | Windows `VirtualAlloc` / Linux `mmap` |
| 多 Chunk 扩展 | 堆空间不足时自动分配新区块，链表管理 |
| 编译期策略切换 | `-DCMEM_STRATEGY=IMPLICIT\|EXPLICIT\|SEGREGATED` |
| operator new/delete 重载 | 可选，透明接入任意 C++ 项目 |

---

## 三种空闲链表策略

### 1. 隐式空闲链表 + First Fit

遍历所有内存块（通过块大小跳转），返回第一个满足要求的空闲块。

```
所有块: [A|alloc][B|free][C|alloc][D|free] ...
                            ^ 找到第一个符合条件的
```

- 复杂度：O(全部块数)
- 优点：实现简单，无需额外指针存储
- 缺点：当堆中存在大量已分配块时，遍历速度慢

### 2. 显式空闲链表 + First Fit

仅在空闲块之间维护双向链表。每个空闲块的载荷存储 `prev`/`next` 指针。

```
空闲块链表: [B] <--> [D] <--> [F] ...
```

- 复杂度：O(空闲块数)
- 优点：只遍历空闲块，比隐式链表快
- 缺点：最坏情况下仍可能遍历整个空闲链表

### 3. 分级空闲链表 (Segregated Free Lists) ★ 默认

将空闲块按大小分到 16 个桶（power-of-2 size class），每个桶内独立维护显式空闲链表。

```
Class  0:     32 B        Class  5:   1024 B       Class 10:  32768 B
Class  1:     64 B        Class  6:   2048 B       Class 11:  65536 B
Class  2:    128 B        Class  7:   4096 B       Class 12: 131072 B
Class  3:    256 B        Class  8:   8192 B       Class 13: 262144 B
Class  4:    512 B        Class  9:  16384 B       ...
```

- 分配：直接定位到目标桶。如果桶为空则向上递升到更大的桶
- 每个桶内使用 First Fit（桶内块大小相近，First Fit 已接近最优）
- 插入：块释放时按大小归入对应桶
- 复杂度：O(桶数) ≈ O(1)，远快于前两种策略

---

## 架构

```
┌───────────────────────────────────────────┐
│             Application Code               │
│  malloc / free / new / delete              │
└──────────────┬────────────────────────────┘
               │
┌──────────────▼────────────────────────────┐
│           Allocator (单例 + API)           │
│  malloc/free/realloc/calloc/stats          │
└──────────────┬────────────────────────────┘
               │
┌──────────────▼────────────────────────────┐
│              Heap Manager                  │
│  · 多 chunk 管理（链式扩展）              │
│  · 边界标签 coalescing (O(1))             │
│  · 分配时分裂 (splitting)                 │
│  · 委托 FreeList 策略                     │
└──────┬─────────────────────┬──────────────┘
       │                     │
       ▼                     ▼
┌──────────────┐    ┌──────────────────────┐
│ ImplicitList │    │ ExplicitFreeList     │
│ + First Fit  │    │ (per size class)     │
│              │    │ + First Fit          │
└──────────────┘    └──────────────────────┘
```

### 内存块布局

```
┌──────────────┬────────────────────────────┬──────────────┐
│ BlockHeader  │      Payload                │    Footer    │
│ (size|flag)  │  (free-list pointers       │  (size|flag) │
│    8 B       │   when block is free)      │     8 B      │
└──────────────┴────────────────────────────┴──────────────┘
```

块首尾各存储一个 `(size | flag)` 字：
- `size`：块总大小（含 header、payload、footer，对齐到 8 字节）
- `flag`：最低位 — 1 表示已分配，0 表示空闲

释放时，通过 `前一个块的尾部` 和 `后一个块的头部` 即可 O(1) 判断相邻块是否空闲，从而决定是否需要合并。这就是边界标签（Boundary Tags）的核心思想。

---

## 核心算法

| 算法 | 复杂度 | 说明 |
|---|---|---|
| 边界标签 (Boundary Tags) | O(1) | 检查块的前后邻居是否空闲 |
| 释放合并 (Coalescing) | O(1) ~ O(N) | 级联合并相邻空闲块 |
| 分裂 (Splitting) | O(1) | 剩余空间 ≥ 最小块时切分 |
| First Fit | O(空闲块数) | 第一个满足要求的块 |
| Segregated Free Lists | O(桶数) | 按 size 分桶，直接定位 |

### 边界标签与合并示例

```
初始：[Prologue|ALLOC] [一个 64KB 的空闲块] [Epilogue|ALLOC]

malloc(10) → 分裂：
  [Prolog][A:32|ALLOC] [剩余 ~64KB|FREE] [Epilogue]

malloc(40) → 再分裂：
  [Prolog][A:32|ALLOC][B:48|ALLOC] [剩余|FREE] [Epilogue]

free(B) → 合并前向：
  [Prolog][A:32|ALLOC][B:48|FREE]→[剩余|FREE] → 合并 → [~64KB|FREE]

free(A) → 合并双向：
  [Prolog][A|FREE]→[B|FREE] → 合并 → [一个 64KB 的空闲块]
```

---

## 性能对比

在 Windows 10 x64, MinGW GCC 10.3, 64KB chunk, Release 编译下测得：

| 基准测试 | cmem (ops/s) | libc (ops/s) | 加速比 |
|---|---|---|---|
| 小对象 8-64B (10 万次) | **~18 M** | ~12 M | **~1.5×** |
| 中对象 128-512B (5 万次) | **~7 M** | ~4 M | **~1.7×** |
| 大对象 1-8KB (5 千次) | **~3.5 M** | ~0.6 M | **~5.8×** |

cmem 的 Segregated Free Lists 在各类场景下均超越系统 `malloc`，大对象场景优势尤为显著（不涉及线程安全开销，且 size class 桶直接定位，搜索路径短）。

---

## 5. 什么时候用？

✅ **游戏 / 实时系统**：需要可预测的低延迟分配，避免系统调用  
✅ **嵌入式环境**：需要精确控制堆空间使用  
✅ **性能压测场景**：特定模式下可超越系统 malloc  
✅ **学习目的**：理解操作系统内存管理底层原理  

❌ **不适用场景**：多线程高并发（本项目未实现线程安全）、内存不足的极端嵌入式环境

---

## 项目结构

```
include/cmem/
├── common.hpp       对齐工具、常量、size|flag 位编码
├── block.hpp        内存块头部/尾部、边界标签导航
├── platform.hpp     OS 内存抽象（VirtualAlloc / mmap）
├── freelist.hpp     三种空闲链表策略声明
├── heap.hpp         堆管理器（chunk、合并、分裂）
└── allocator.hpp    公开 API、Stats 结构

src/
├── platform.cpp     跨平台 OS 内存申请与释放
├── freelist.cpp     空闲链表实现（Implicit / Explicit / Segregated）
├── heap.cpp         核心分配/释放/合并/分裂/扩展
├── allocator.cpp    Allocator 单例 + 公开 API 实现
├── overrides.cpp    可选 ::operator new/delete 重载（独立编译）
└── main.cpp         功能测试（冒烟 + 压力）

benchmarks/
└── benchmark.cpp    与系统 malloc 的性能对比

CMakeLists.txt      构建配置文件
run_benchmark.bat   一键构建 + 测试 + 对比
README.md          本文件
```

---

## 构建与使用

### 前置条件

- CMake ≥ 3.16
- C++17 编译器（GCC / Clang / MSVC）

### 快速运行

```bash
# 方式一：Windows 双击
run_benchmark.bat

# 方式二：手动构建（默认 SegregatedFreeLists）
cmake -B build -DCMAKE_BUILD_TYPE=Release -DCMEM_STRATEGY=SEGREGATED
cd build
make                # 或 mingw32-make
./cmem_demo         # 功能测试
./cmem_bench        # 性能对比
```

### 切换分配策略

```bash
cmake -B build -DCMEM_STRATEGY=IMPLICIT     # 隐式链表 + First Fit
cmake -B build -DCMEM_STRATEGY=EXPLICIT     # 显式链表 + First Fit
cmake -B build -DCMEM_STRATEGY=SEGREGATED   # 分级空闲链表（推荐/默认）
```

### 全局 operator new/delete 重载

额外编译 `src/overrides.cpp` 并链接，所有 C++ 的 `new`/`delete`（含 `std::vector`、`std::string` 等）将自动使用 cmem：

```bash
g++ -c src/overrides.cpp -Iinclude -std=c++17
g++ -o myapp myapp.cpp overrides.cpp -Lbuild -lcmem
```

---

## 参考

- CMU 15-213: Introduction to Computer Systems — [课程主页](https://www.cs.cmu.edu/~213/)
- CS:APP Malloc Lab — [实验说明](http://csapp.cs.cmu.edu/3e/malloclab.pdf)
- dlmalloc / ptmalloc 设计
- jemalloc: Facebook 的高性能分配器
