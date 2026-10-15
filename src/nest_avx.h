#ifndef NEST_AVX_H
#define NEST_AVX_H

typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;
typedef unsigned long long uint64_t;

//navx_init(void *memory_buffer, uint64_t memory_size) initializes the navx engine and will only use the given memory
void navx_init(void *memory_buffer, uint64_t memory_size);

//navx_add_part(int part_id, float *vertices, int vertex_count, int quantity) will add the given part to the engine
void navx_add_part(int part_id, float *vertices, int vertex_count, int quantity);

//navx_solve(float sheet_width, float sheet_height, float *out_x, float *out_y, float *out_rotation) will calculate the most efficient way for placing the parts in the given grid using SIMD
void navx_solve(float sheet_width, float sheet_height, float *out_x, float *out_y, float *out_rotation);

//navx_free() resets the engine. navx_init isn't required again
void navx_free();

#endif