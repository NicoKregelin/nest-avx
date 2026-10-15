# Nest-AVX - High-Performance Nesting C Engine

**Nest-AVX** is an ultra-high-performance, header-only-style 2D nesting and polygon collision library written in pure C. Designed for CAM software, laser/plasma cutting systems, and real-time layout optimization, it combines hardware-accelerated SIMD with a deterministic arena allocator to deliver sub-millisecond packing solutions.

For commercial licensing and inquiries, visit **[nest-avx.nicokregelin.workers.dev](https://nest-avx.nicokregelin.workers.dev)**.

## Key Features

- **Blazing Fast:** Nests 24 complex, multi-vertex concave parts (including 90°/180° rotations) in **176 microseconds**.
- **Multi-Architecture SIMD:** Hardware-accelerated overlap testing using **AVX-512**, **AVX2**, and **ARM NEON**.
- **Bring-Your-Own-Memory (BYOM):** Zero dynamic heap allocations during runtime (`malloc`/`free` are never called inside the solver). Powered by a flat, cache-friendly memory arena.
- **Advanced Heuristics:** Largest-first area sorting and automatic duplicate-part positioning skip redundant search space.
- **Zero Dependencies:** Written in standard ISO C with zero external libraries.

---

## Benchmark

Tested on a 40.0 x 25.0 sheet (80 x 50 grid pixels) packing 24 parts (12 squares, 8 bars, 4 L-brackets):

| Metric | Performance |
|---|---|
| **Total Parts Nested** | 24 instances |
| **Execution Latency** | **0.176 ms (176 µs)** |
| **Overlap Errors** | 0 |
| **Memory Overhead** | Flat pre-allocated arena |

---

## API Reference (`nest_avx.h`)

```c
#ifndef NEST_AVX_H
#define NEST_AVX_H

typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;
typedef unsigned long long uint64_t;

// Initializes the navx engine using the provided pre-allocated memory buffer
void navx_init(void *memory_buffer, uint64_t memory_size);

// Adds a part polygon with its vertices, vertex count, and quantity
void navx_add_part(int part_id, float *vertices, int vertex_count, int quantity);

// Solves the optimal sheet packing layout using SIMD collision detection
void navx_solve(float sheet_width, float sheet_height, float *out_x, float *out_y, float *out_rotation);

// Resets the engine state (navx_init does not need to be called again)
void navx_free(void);

#endif