#pragma once

#include <stddef.h>

#include "common/list.h"

#define KMEM_BLOCK_MAGIC 0xABCDDCBA

#define KMEM_FREE_BLOCK_PTR(list_ptr) container_of(list_ptr, kmem_free_block_t, list_node)

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    struct intrusive_list list_node;
    size_t size;
} kmem_free_block_t;

typedef struct {
    uint32_t magic;
    struct intrusive_list *prev_free;
    size_t size;
} kmem_used_block_t;

typedef struct {
    void *start;
    size_t size;
    struct intrusive_list *free_list;
} kmem_heap_t;

void kmem_init(kmem_heap_t *heap_desc, void *heap_start, size_t heap_size);
void *kmem_alloc(kmem_heap_t *heap_desc, size_t size);
void kmem_free(kmem_heap_t *heap_desc, void *ptr);

#ifdef __cplusplus
}
#endif
