#ifndef FIB_HEAP_H
#define FIB_HEAP_H

#include <stddef.h>

typedef struct FibNode FibNode;
typedef struct FibHeap FibHeap;

struct FibNode {
    size_t key;
    int class_index;
    int degree;
    int mark;
    FibNode *parent;
    FibNode *child;
    FibNode *left;
    FibNode *right;
};

struct FibHeap {
    FibNode *min;
    int n;
};

void fib_heap_init(FibHeap *heap);
void fib_heap_insert(FibHeap *heap, FibNode *node);
FibNode *fib_heap_min(FibHeap *heap);
void fib_heap_remove(FibHeap *heap, FibNode *node);
void fib_heap_decrease_key(FibHeap *heap, FibNode *node, size_t new_key);
FibNode *fib_heap_extract_min(FibHeap *heap);

#endif
