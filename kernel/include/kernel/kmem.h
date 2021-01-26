#pragma once

#include <stddef.h>

typedef struct {
    void *start;
    size_t size;
} kmem_heap_t;

void kmem_init(kmem_heap_t *heap_desc, void *heap_start, size_t heap_size);
void *kmem_alloc(size_t size);
void *kmem_free(void *ptr);
