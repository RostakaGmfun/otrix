#pragma once

#include <stddef.h>

#include "common/list.h"

#define KMEM_BLOCK_MAGIC 0xDEADBEEFABCDDCBALU

#define KMEM_FREE_BLOCK_PTR(list_ptr) container_of(list_ptr, kmem_free_block_t, list_node)

#define KMEM_MAX(a, b) ((a) > (b) ? (a) : (b))
#define KMEM_MIN_BLOCK_SIZE (KMEM_MAX(sizeof(kmem_free_block_t), sizeof(kmem_used_block_t)))
#define KMEM_MIN_FREE_BLOCK_SIZE (KMEM_MIN_BLOCK_SIZE * 2)
#define KMEM_ALLOCATION_SIZE(size) KMEM_MAX(size, KMEM_MIN_FREE_BLOCK_SIZE)

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint64_t magic;
    struct intrusive_list list_node;
    size_t size; /**< Full size including KMEM_MIN_BLOCK_SIZE for block header **/
} kmem_free_block_t;

typedef struct {
    uint64_t magic;
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

void kmem_print_free(kmem_heap_t *heap_desc);

#ifdef __cplusplus
}
#endif
