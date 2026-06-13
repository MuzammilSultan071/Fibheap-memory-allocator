#include "fib_heap.h"
#include <stdlib.h>

static void list_insert_after(FibNode *pos, FibNode *node) {
    node->right = pos->right;
    node->left = pos;
    pos->right->left = node;
    pos->right = node;
}

static void list_remove(FibNode *node) {
    node->left->right = node->right;
    node->right->left = node->left;
    node->left = node;
    node->right = node;
}

static void heap_link(FibHeap *heap, FibNode *y, FibNode *x) {
    list_remove(y);
    y->parent = x;
    y->mark = 0;
    if (!x->child) {
        x->child = y;
        y->left = y;
        y->right = y;
    } else {
        list_insert_after(x->child, y);
    }
    x->degree++;
    (void)heap;
}

static void consolidate(FibHeap *heap) {
    if (!heap->min) return;

    int max_degree = 64;
    FibNode *A[64];
    for (int i = 0; i < max_degree; i++) A[i] = NULL;

    FibNode *start = heap->min;
    FibNode *w = start;
    do {
        FibNode *x = w;
        FibNode *next = w->right;
        int d = x->degree;

        while (A[d]) {
            FibNode *y = A[d];
            if (y->key < x->key) {
                FibNode *tmp = x;
                x = y;
                y = tmp;
            }
            heap_link(heap, y, x);
            A[d] = NULL;
            d++;
        }
        A[d] = x;
        w = next;
    } while (w != start);

    heap->min = NULL;
    for (int i = 0; i < max_degree; i++) {
        if (A[i]) {
            A[i]->left = A[i];
            A[i]->right = A[i];
            if (!heap->min) {
                heap->min = A[i];
            } else {
                list_insert_after(heap->min, A[i]);
                if (A[i]->key < heap->min->key) heap->min = A[i];
            }
        }
    }
}

void fib_heap_init(FibHeap *heap) {
    heap->min = NULL;
    heap->n = 0;
}

void fib_heap_insert(FibHeap *heap, FibNode *node) {
    node->parent = NULL;
    node->child = NULL;
    node->degree = 0;
    node->mark = 0;
    node->left = node;
    node->right = node;

    if (!heap->min) {
        heap->min = node;
    } else {
        list_insert_after(heap->min, node);
        if (node->key < heap->min->key) heap->min = node;
    }
    heap->n++;
}

FibNode *fib_heap_min(FibHeap *heap) {
    return heap->min;
}

static void cut(FibHeap *heap, FibNode *x, FibNode *y) {
    if (x->right == x) {
        y->child = NULL;
    } else {
        if (y->child == x) y->child = x->right;
        list_remove(x);
    }
    y->degree--;
    x->parent = NULL;
    x->mark = 0;
    list_insert_after(heap->min, x);
}

static void cascading_cut(FibHeap *heap, FibNode *y) {
    FibNode *z = y->parent;
    if (z) {
        if (!y->mark) {
            y->mark = 1;
        } else {
            cut(heap, y, z);
            cascading_cut(heap, z);
        }
    }
}

void fib_heap_decrease_key(FibHeap *heap, FibNode *node, size_t new_key) {
    if (new_key > node->key) return;
    node->key = new_key;
    FibNode *p = node->parent;
    if (p && node->key < p->key) {
        cut(heap, node, p);
        cascading_cut(heap, p);
    }
    if (heap->min == NULL || node->key < heap->min->key) heap->min = node;
}

FibNode *fib_heap_extract_min(FibHeap *heap) {
    FibNode *z = heap->min;
    if (!z) return NULL;

    if (z->child) {
        FibNode *c = z->child;
        FibNode *start = c;
        do {
            FibNode *next = c->right;
            c->parent = NULL;
            list_insert_after(z, c);
            c = next;
        } while (c != start);
        z->child = NULL;
    }

    if (z->right == z) {
        heap->min = NULL;
    } else {
        heap->min = z->right;
        list_remove(z);
        consolidate(heap);
    }
    heap->n--;
    z->left = z;
    z->right = z;
    z->parent = NULL;
    z->child = NULL;
    return z;
}

void fib_heap_remove(FibHeap *heap, FibNode *node) {
    fib_heap_decrease_key(heap, node, 0);
    fib_heap_extract_min(heap);
}
