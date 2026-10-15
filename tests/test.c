#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include <nest_avx.h>

// High-resolution timer in milliseconds
static double get_time_ms(void)
{
#if defined(_WIN32)
    struct timespec ts;
    timespec_get(&ts, TIME_UTC);
    return (double)ts.tv_sec * 1000.0 + (double)ts.tv_nsec / 1000000.0;
#else
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec * 1000.0 + (double)ts.tv_nsec / 1000000.0;
#endif
}

// Helper to point-in-polygon check for clean ASCII rendering of each placed part ID
static int point_in_poly(float px, float py, const float *verts, int nverts, float ox, float oy, float rot_deg)
{
    int inside = 0;
    for (int i = 0, j = nverts - 1; i < nverts; j = i++)
    {
        float vx1 = verts[i * 2], vy1 = verts[i * 2 + 1];
        float vx2 = verts[j * 2], vy2 = verts[j * 2 + 1];
        float rx1, ry1, rx2, ry2;

        if (rot_deg == 0.0f)        { rx1 =  vx1; ry1 =  vy1; rx2 =  vx2; ry2 =  vy2; }
        else if (rot_deg == 90.0f)  { rx1 = -vy1; ry1 =  vx1; rx2 = -vy2; ry2 =  vx2; }
        else if (rot_deg == 180.0f) { rx1 = -vx1; ry1 = -vy1; rx2 = -vx2; ry2 = -vy2; }
        else                        { rx1 =  vy1; ry1 = -vx1; rx2 =  vy2; ry2 = -vx2; }

        float x1 = rx1 + ox, y1 = ry1 + oy;
        float x2 = rx2 + ox, y2 = ry2 + oy;

        if (((y1 > py) != (y2 > py)) &&
            (px < (x2 - x1) * (py - y1) / (y2 - y1) + x1))
        {
            inside = !inside;
        }
    }
    return inside;
}

int main(void)
{
    // 1. Allocate 64 MB arena
    uint64_t arena_size = 64ULL * 1024ULL * 1024ULL;
    void *memory = malloc(arena_size);
    if (!memory) return 1;

    // 2. Define 3 distinct part geometries:
    // Part 0: Small 4x4 square (quantity 12) - added FIRST to test largest-first sorting!
    float square_verts[] = {
        0.0f, 0.0f,
        4.0f, 0.0f,
        4.0f, 4.0f,
        0.0f, 4.0f
    };

    // Part 1: Long 3x14 bar (quantity 8) - tests 90-degree rotation!
    float bar_verts[] = {
        0.0f, 0.0f,
        14.0f, 0.0f,
        14.0f, 3.0f,
        0.0f, 3.0f
    };

    // Part 2: Large 10x10 L-bracket (quantity 4) - concave polygon!
    float l_verts[] = {
        0.0f,  0.0f,
        10.0f, 0.0f,
        10.0f, 4.0f,
        4.0f,  4.0f,
        4.0f,  10.0f,
        0.0f,  10.0f
    };

    const int total_instances = 12 + 8 + 4; // 24 total parts
    float out_x[24] = {0};
    float out_y[24] = {0};
    float out_rot[24] = {0};

    float sheet_w = 40.0f; // 80 pixels wide at RESOLUTION 0.5f
    float sheet_h = 25.0f; // 50 pixels high at RESOLUTION 0.5f

    // 3. Run & Time navx_init + navx_add_part + navx_solve
    double t0 = get_time_ms();

    navx_init(memory, arena_size);
    navx_add_part(1, square_verts, 4, 12);
    navx_add_part(2, bar_verts,    4, 8);
    navx_add_part(3, l_verts,      6, 4);
    navx_solve(sheet_w, sheet_h, out_x, out_y, out_rot);

    double t1 = get_time_ms();
    double elapsed_ms = t1 - t0;

    printf("=== NEST_AVX BENCHMARK RESULTS ===\n");
    printf("Total Parts Nested : %d (4 L-Brackets, 8 Bars, 12 Squares)\n", total_instances);
    printf("Sheet Dimensions   : %.1f x %.1f (Grid: %d x %d pixels)\n",
           sheet_w, sheet_h, (int)(sheet_w / 0.5f), (int)(sheet_h / 0.5f));
    printf("Execution Time     : %.4f ms (%.1f microseconds)\n\n", elapsed_ms, elapsed_ms * 1000.0);

    printf("Inst | Type      | Out X  | Out Y  | Rotation\n");
    printf("-----+-----------+--------+--------+---------\n");
    for (int i = 0; i < total_instances; i++)
    {
        const char *name = (i < 12) ? "Square" : (i < 20) ? "Bar" : "L-Bracket";
        printf("%4d | %-9s | %6.2f | %6.2f | %5.1f deg\n",
               i, name, out_x[i], out_y[i], out_rot[i]);
    }

    // 4. Print ASCII Sheet Visualization (1 char = 1 grid cell = 0.5f units)
    int grid_w = (int)(sheet_w / 0.5f);
    int grid_h = (int)(sheet_h / 0.5f);

    printf("\n=== PACKED SHEET BITMAP (%d x %d grid) ===\n", grid_w, grid_h);
    printf("Legend: [L] = L-Bracket, [B] = Bar, [S] = Square, [.] = Empty Sheet, [!] = OVERLAP BUG\n\n");

    int overlap_errors = 0;
    for (int gy = 0; gy < grid_h; gy++)
    {
        printf("%2d | ", gy);
        for (int gx = 0; gx < grid_w; gx++)
        {
            float px = (gx + 0.5f) * 0.5f;
            float py = (gy + 0.5f) * 0.5f;
            int hit_count = 0;
            char symbol = '.';

            for (int i = 0; i < total_instances; i++)
            {
                const float *v = (i < 12) ? square_verts : (i < 20) ? bar_verts : l_verts;
                int nv = (i < 20) ? 4 : 6;
                char s = (i < 12) ? 'S' : (i < 20) ? 'B' : 'L';

                if (point_in_poly(px, py, v, nv, out_x[i], out_y[i], out_rot[i]))
                {
                    hit_count++;
                    symbol = s;
                }
            }
            if (hit_count > 1)
            {
                symbol = '!';
                overlap_errors++;
            }
            putchar(symbol);
        }
        printf(" |\n");
    }

    printf("\nOverlap Errors Detected: %d\n", overlap_errors);

    navx_free();
    free(memory);
    return 0;
}