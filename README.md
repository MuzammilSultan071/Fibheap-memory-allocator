# Large-Scale Best-Fit Heap Allocator Using Fibonacci-Style Free List Management

A user-level heap allocator for large virtual address spaces that combines size-class based free lists with a Fibonacci-style meta-structure to support best-fit-style allocation, efficient coalescing, and low-fragmentation memory management.

## Overview

This project implements a custom heap allocator designed for workloads that mix many small allocations with occasional large allocations. The allocator focuses on reducing both internal and external fragmentation while keeping allocation and free operations efficient in practice.

Instead of scanning a single global free list linearly, the allocator organizes free memory into multiple size classes. A Fibonacci-style structure is used as the meta-layer to quickly identify the smallest usable free block for a requested allocation size.

The project also includes benchmark workloads that simulate realistic memory patterns such as web server traffic, database buffer pools, and game engine object pools.

## Key Features

- Custom user-level allocator with `my_malloc`, `my_free`, and `my_realloc`.
- Multiple free lists organized by size class.
- Fibonacci-style meta-structure for best-fit-ish block selection.
- Block splitting and adjacent block coalescing.
- Lazy consolidation to reduce allocation overhead.
- Benchmark suite for realistic mixed-size workloads.
- Fragmentation and latency measurement support.
- Metadata overhead tracking.

## Design Goals

The allocator is built around these goals:

- Support allocations from small blocks to multi-megabyte requests.
- Keep fragmentation low under mixed workloads.
- Avoid expensive linear scans over all free blocks.
- Reduce per-allocation overhead using lazy updates.
- Keep block metadata compact enough for large heaps.

## Architecture

The project is split into the following modules:

- `allocator.h` / `allocator.c`  
  Core heap allocator implementation.

- `fib_heap.h` / `fib_heap.c`  
  Fibonacci-style meta-structure used to organize size classes.

- `benchmark.c`  
  Benchmark runner for workload simulation and timing.

- `Makefile`  
  Build configuration.

## How It Works

1. `my_malloc(size)` maps the request into an appropriate size class.
2. The allocator queries the meta-structure to find the smallest usable class.
3. A free block is selected, split if necessary, and returned to the caller.
4. `my_free(ptr)` marks the block as free and coalesces with neighboring free blocks when possible.
5. `my_realloc(ptr, new_size)` tries to preserve the existing block when possible, otherwise allocates a new block and copies the data.
6. Free-block updates are handled lazily to avoid unnecessary work on every operation.

## File Layout

```text
.
├── allocator.h
├── allocator.c
├── fib_heap.h
├── fib_heap.c
├── benchmark.c
├── Makefile
└── README.md
```

## Build Instructions

### Requirements
- GCC or Clang
- GNU Make
- Linux or a POSIX-compatible environment

### Build
```bash
make
```

This produces the benchmark executable.

### Clean
```bash
make clean
```

## Usage

### Run Benchmark
```bash
./bench
```

The benchmark prints:
- total heap size,
- total free memory,
- peak used memory,
- average allocation latency,
- average free latency.

### Integrate the Allocator
Use the allocator functions in your own C program:

```c
#include "allocator.h"

int main(void) {
    allocator_init();

    void *p = my_malloc(256);
    p = my_realloc(p, 512);
    my_free(p);

    allocator_destroy();
    return 0;
}
```

## Benchmark Workloads

The benchmark suite is designed to simulate common allocation patterns:

- **Web server style workload**  
  Many small allocations, short-lived request objects, and occasional larger buffers.

- **Database buffer pool workload**  
  Repeated medium-sized allocations with predictable reuse behavior.

- **Game engine object pool workload**  
  Frequent small and medium allocations with bursts of activity.

## Metrics Collected

The project measures the following:

- **Fragmentation ratio over time**
- **Allocation latency distribution**
- **Free latency distribution**
- **Peak memory usage**
- **Metadata overhead**
- **Heap utilization**

## Why This Design

A simple global free list becomes expensive when the heap contains many free blocks. Buddy allocation can be fast, but it often introduces internal fragmentation because it rounds sizes aggressively.

This allocator is designed to sit between those extremes:

- more flexible than buddy allocation,
- more structured than a plain free list,
- more fragmentation-aware than simple first-fit designs.

## Limitations

This is an educational allocator project and not a production-grade system allocator. Current limitations may include:

- simplified heap growth strategy,
- simplified metadata handling,
- limited OS integration,
- no multithreaded synchronization,
- benchmark coverage can still be extended.

## Future Improvements

Possible next steps include:

- boundary tags for stronger coalescing,
- more accurate fragmentation tracking,
- per-thread arenas,
- larger workload traces,
- CSV export for benchmark results,
- graphs for latency and fragmentation trends,
- better class selection heuristics.

## Resume Highlights

This project demonstrates:

- systems programming in C,
- custom memory management,
- allocator design and optimization,
- benchmark engineering,
- fragmentation analysis,
- low-level data structure implementation.

## License

Add a license before publishing if needed.

## Author

Developed as a custom memory allocator project.
