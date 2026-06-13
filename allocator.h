#ifndef ALLOCATOR_H
#define ALLOCATOR_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

void allocator_init(void);
void allocator_destroy(void);

void *my_malloc(size_t size);
void my_free(void *ptr);
void *my_realloc(void *ptr, size_t new_size);

size_t allocator_total_heap_size(void);
size_t allocator_total_free_size(void);
size_t allocator_peak_used_size(void);

#ifdef __cplusplus
}
#endif

#endif
