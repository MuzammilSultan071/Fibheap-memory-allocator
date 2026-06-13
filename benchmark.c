#include "allocator.h"
#include <stdio.h>
#include <stdlib.h>

#ifdef _WIN32
#include <windows.h>

static double now_seconds(void) {
    static LARGE_INTEGER freq;
    static int init = 0;
    LARGE_INTEGER t;
    if (!init) {
        QueryPerformanceFrequency(&freq);
        init = 1;
    }
    QueryPerformanceCounter(&t);
    return (double)t.QuadPart / (double)freq.QuadPart;
}
#else
#include <time.h>

static double now_seconds(void) {
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC, &t);
    return (double)t.tv_sec + (double)t.tv_nsec / 1e9;
}
#endif

#define OPS 200000
#define MAX_PTRS 50000

static double ns_diff(double a, double b) {
    return (b - a) * 1000000000.0;
}

int main(void) {
    allocator_init();

    void **ptrs = calloc(MAX_PTRS, sizeof(void *));
    size_t *sizes = calloc(MAX_PTRS, sizeof(size_t));
    if (!ptrs || !sizes) return 1;

    srand(42);

    double alloc_ns = 0;
    double free_ns = 0;

    for (int i = 0; i < OPS; i++) {
        int idx = rand() % MAX_PTRS;

        if (ptrs[idx] && (rand() % 100) < 45) {
            double t1 = now_seconds();
            my_free(ptrs[idx]);
            double t2 = now_seconds();
            free_ns += ns_diff(t1, t2);
            ptrs[idx] = NULL;
            sizes[idx] = 0;
        } else {
            size_t sz;
            int r = rand() % 100;
            if (r < 70) sz = 64 + (rand() % 960);
            else if (r < 95) sz = 1024 + (rand() % 64512);
            else sz = 1 << (20 + (rand() % 3));

            double t1 = now_seconds();
            void *p = my_malloc(sz);
            double t2 = now_seconds();
            alloc_ns += ns_diff(t1, t2);

            if (p) {
                ptrs[idx] = p;
                sizes[idx] = sz;
            }
        }
    }

    for (int i = 0; i < MAX_PTRS; i++) {
        if (ptrs[i]) my_free(ptrs[i]);
    }

    printf("heap_size=%zu\n", allocator_total_heap_size());
    printf("free_size=%zu\n", allocator_total_free_size());
    printf("peak_used=%zu\n", allocator_peak_used_size());
    printf("alloc_avg_ns=%.0f\n", alloc_ns / OPS);
    printf("free_avg_ns=%.0f\n", free_ns / OPS);

    free(ptrs);
    free(sizes);
    allocator_destroy();
    return 0;
}
