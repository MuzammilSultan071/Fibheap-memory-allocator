#include "allocator.h"
#include "fib_heap.h"
#ifdef _WIN32
#include <windows.h>
#else
#include <sys/mman.h>
#include <unistd.h>
#endif
#include <stdint.h>
#include <string.h>


#define HEAP_SIZE (64ULL * 1024 * 1024)
#define ALIGNMENT 16
#define MIN_BLOCK_SIZE 64
#define SIZE_CLASS_COUNT 48

typedef struct BlockHeader BlockHeader;

struct BlockHeader {
    size_t size;
    size_t prev_size;
    int free;
    int class_index;
    BlockHeader *prev_free;
    BlockHeader *next_free;
    FibNode fib_node;
};

static void *heap_base = NULL;
static size_t heap_size = 0;
static size_t used_bytes = 0;
static size_t peak_used = 0;

static BlockHeader *free_lists[SIZE_CLASS_COUNT];
static FibHeap class_heap;
static FibNode class_nodes[SIZE_CLASS_COUNT];
static int class_active[SIZE_CLASS_COUNT];

static size_t align_up(size_t n) {
    return (n + (ALIGNMENT - 1)) & ~(ALIGNMENT - 1);
}

static size_t class_size_from_index(int idx) {
    size_t a = MIN_BLOCK_SIZE;
    size_t b = 96;
    if (idx == 0) return a;
    if (idx == 1) return b;
    for (int i = 2; i <= idx; i++) {
        size_t c = a + b;
        a = b;
        b = c;
    }
    return b;
}

static int size_to_class(size_t size) {
    size_t want = size < MIN_BLOCK_SIZE ? MIN_BLOCK_SIZE : size;
    size_t a = MIN_BLOCK_SIZE;
    size_t b = 96;
    if (want <= a) return 0;
    if (want <= b) return 1;
    for (int i = 2; i < SIZE_CLASS_COUNT; i++) {
        size_t c = a + b;
        if (want <= c) return i;
        a = b;
        b = c;
    }
    return SIZE_CLASS_COUNT - 1;
}

static void push_free(BlockHeader *blk) {
    int idx = blk->class_index;
    blk->free = 1;
    blk->prev_free = NULL;
    blk->next_free = free_lists[idx];
    if (free_lists[idx]) free_lists[idx]->prev_free = blk;
    free_lists[idx] = blk;
}

static void remove_free(BlockHeader *blk) {
    int idx = blk->class_index;
    if (blk->prev_free) blk->prev_free->next_free = blk->next_free;
    else free_lists[idx] = blk->next_free;
    if (blk->next_free) blk->next_free->prev_free = blk->prev_free;
    blk->prev_free = NULL;
    blk->next_free = NULL;
    blk->free = 0;
}

static BlockHeader *split_block(BlockHeader *blk, size_t need) {
    size_t total = blk->size;
    if (total < need + sizeof(BlockHeader) + MIN_BLOCK_SIZE) return blk;

    size_t remain = total - need;
    blk->size = need;

    BlockHeader *new_blk = (BlockHeader *)((char *)blk + need);
    new_blk->size = remain;
    new_blk->prev_size = need;
    new_blk->free = 1;
    new_blk->class_index = size_to_class(remain);
    new_blk->prev_free = NULL;
    new_blk->next_free = NULL;

    char *after = (char *)new_blk + remain;
    if ((char *)after < (char *)heap_base + heap_size) {
        BlockHeader *next = (BlockHeader *)after;
        next->prev_size = remain;
    }

    push_free(new_blk);
    return blk;
}

static BlockHeader *coalesce(BlockHeader *blk) {
    char *base = (char *)heap_base;
    char *end = base + heap_size;

    BlockHeader *left = NULL;
    BlockHeader *right = NULL;

    if ((char *)blk > base + sizeof(BlockHeader)) {
        left = (BlockHeader *)((char *)blk - blk->prev_size);
        if (left->free) {
            remove_free(left);
            left->size += blk->size;
            blk = left;
        }
    }

    char *next_addr = (char *)blk + blk->size;
    if (next_addr < end) {
        right = (BlockHeader *)next_addr;
        if (right->free) {
            remove_free(right);
            blk->size += right->size;
        }
    }

    char *after = (char *)blk + blk->size;
    if (after < end) {
        BlockHeader *n = (BlockHeader *)after;
        n->prev_size = blk->size;
    }

    return blk;
}

void allocator_init(void) {
    if (heap_base) return;

    heap_size = HEAP_SIZE;
    #ifdef _WIN32
heap_base = VirtualAlloc(NULL, heap_size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
if (!heap_base) {
    heap_size = 0;
    return;
}
#else
heap_base = mmap(NULL, heap_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
if (heap_base == MAP_FAILED) {
    heap_base = NULL;
    heap_size = 0;
    return;
}
#endif

    memset(free_lists, 0, sizeof(free_lists));
    fib_heap_init(&class_heap);
    memset(class_active, 0, sizeof(class_active));

    for (int i = 0; i < SIZE_CLASS_COUNT; i++) {
        class_nodes[i].class_index = i;
        class_nodes[i].key = (size_t)-1;
        class_nodes[i].left = &class_nodes[i];
        class_nodes[i].right = &class_nodes[i];
    }

    BlockHeader *first = (BlockHeader *)heap_base;
    first->size = heap_size;
    first->prev_size = 0;
    first->free = 1;
    first->class_index = size_to_class(heap_size);
    first->prev_free = NULL;
    first->next_free = NULL;
    first->fib_node.key = heap_size;
    first->fib_node.class_index = first->class_index;

    push_free(first);
    class_nodes[first->class_index].key = class_size_from_index(first->class_index);
    fib_heap_insert(&class_heap, &class_nodes[first->class_index]);
    class_active[first->class_index] = 1;
}

void allocator_destroy(void) {
    #ifdef _WIN32
if (heap_base) VirtualFree(heap_base, 0, MEM_RELEASE);
#else
if (heap_base) munmap(heap_base, heap_size);
#endif
    heap_base = NULL;
    heap_size = 0;
    used_bytes = 0;
    peak_used = 0;
}

static BlockHeader *find_block(size_t need) {
    int idx = size_to_class(need);
    for (int i = idx; i < SIZE_CLASS_COUNT; i++) {
        BlockHeader *cur = free_lists[i];
        while (cur) {
            if (cur->size >= need) return cur;
            cur = cur->next_free;
        }
    }
    return NULL;
}

void *my_malloc(size_t size) {
    if (size == 0) return NULL;
    if (!heap_base) allocator_init();
    if (!heap_base) return NULL;

    size_t need = align_up(size + sizeof(BlockHeader));
    if (need < MIN_BLOCK_SIZE) need = MIN_BLOCK_SIZE;

    BlockHeader *blk = find_block(need);
    if (!blk) return NULL;

    remove_free(blk);
    blk = split_block(blk, need);
    blk->free = 0;

    used_bytes += blk->size;
    if (used_bytes > peak_used) peak_used = used_bytes;

    return (void *)((char *)blk + sizeof(BlockHeader));
}

void my_free(void *ptr) {
    if (!ptr || !heap_base) return;

    BlockHeader *blk = (BlockHeader *)((char *)ptr - sizeof(BlockHeader));
    if (blk->free) return;

    used_bytes -= blk->size;
    blk->free = 1;
    blk = coalesce(blk);
    blk->class_index = size_to_class(blk->size);
    push_free(blk);
}

void *my_realloc(void *ptr, size_t new_size) {
    if (!ptr) return my_malloc(new_size);
    if (new_size == 0) {
        my_free(ptr);
        return NULL;
    }

    BlockHeader *blk = (BlockHeader *)((char *)ptr - sizeof(BlockHeader));
    size_t old_payload = blk->size - sizeof(BlockHeader);
    if (new_size <= old_payload) return ptr;

    void *new_ptr = my_malloc(new_size);
    if (!new_ptr) return NULL;

    size_t copy_size = old_payload < new_size ? old_payload : new_size;
    memcpy(new_ptr, ptr, copy_size);
    my_free(ptr);
    return new_ptr;
}

size_t allocator_total_heap_size(void) {
    return heap_size;
}

size_t allocator_total_free_size(void) {
    size_t total = 0;
    for (int i = 0; i < SIZE_CLASS_COUNT; i++) {
        BlockHeader *cur = free_lists[i];
        while (cur) {
            total += cur->size;
            cur = cur->next_free;
        }
    }
    return total;
}

size_t allocator_peak_used_size(void) {
    return peak_used;
}
