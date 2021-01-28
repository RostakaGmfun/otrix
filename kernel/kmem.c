#include "kernel/kmem.h"

#include <string.h>

void kmem_init(kmem_heap_t *heap_desc, void *heap_start, size_t heap_size) {
    memset(heap_desc, 0, sizeof(kmem_heap_t));
    heap_desc->start = heap_start;
    heap_desc->size = heap_size;

    kmem_free_block_t *first_free = heap_start;
    intrusive_list_init(&first_free->list_node);
    first_free->size = heap_size - sizeof(kmem_used_block_t);
    heap_desc->free_list = &first_free->list_node;
}

void *kmem_alloc(kmem_heap_t *heap_desc, size_t size) {
    struct intrusive_list *p_free = heap_desc->free_list;
    do {
        if (size >= KMEM_FREE_BLOCK_PTR(p_free)->size) {
            break;
        }
        p_free = p_free->next;
    } while (p_free != heap_desc->free_list);

    if (KMEM_FREE_BLOCK_PTR(p_free)->size < size) {
        return NULL;
    }

    kmem_used_block_t *p_block = (void *)KMEM_FREE_BLOCK_PTR(p_free);
    uint8_t *p_after = (uint8_t *)p_block + sizeof(kmem_used_block_t) + size;

    struct intrusive_list *prev = NULL;
    if (p_free == heap_desc->free_list) {
        if ((uint8_t *)heap_desc->start + heap_desc->size - p_after < sizeof(kmem_free_block_t)) {
            heap_desc->free_list = NULL;
        } else {
            kmem_free_block_t *new_free = (kmem_free_block_t *)p_after;
            new_free->size = KMEM_FREE_BLOCK_PTR(p_free)->size - size - sizeof(kmem_used_block_t);
            intrusive_list_init(&new_free->list_node);
            heap_desc->free_list = &new_free->list_node;
            prev = &new_free->list_node;
        }
    } else {
        prev = p_free->prev;
        // Sufficient space to allocate another free block?
        if ((uint8_t *)heap_desc->start + heap_desc->size - p_after > sizeof(kmem_free_block_t)) {
            kmem_free_block_t *new_free = (kmem_free_block_t *)p_after;
            intrusive_list_init(&new_free->list_node);
            new_free->size = KMEM_FREE_BLOCK_PTR(p_free)->size - size - sizeof(kmem_used_block_t);
            intrusive_list_unlink_node(p_free);
            intrusive_list_insert(prev, &new_free->list_node);
        } else {
            intrusive_list_unlink_node(p_free);
        }
    }

    p_block->prev_free = prev;
    p_block->magic = KMEM_BLOCK_MAGIC;
    p_block->size = size;

    return (void *)(p_block + 1);
}

void kmem_free(kmem_heap_t *heap_desc, void *ptr) {
    kmem_used_block_t *p_block = (kmem_used_block_t *)ptr - 1;
    if ((void *)p_block < heap_desc->start || (void *)p_block > heap_desc->start + heap_desc->size) {
        return;
    }

    if (KMEM_BLOCK_MAGIC != p_block->magic) {
        return;
    }

    const size_t block_size = p_block->size;

    if (NULL == p_block->prev_free) {
        kASSERT((void *)p_block == heap_desc->start);
        kmem_free_block_t *p_free = (void *)p_block;
        intrusive_list_init(&p_free->list_node);
        p_free->size = heap_desc->size - sizeof(kmem_used_block_t);
        heap_desc->free_list = &p_free->list_node;
    } else {
        struct intrusive_list *p_prev_free = p_block->prev_free;
        if ((uint8_t *)KMEM_FREE_BLOCK_PTR(p_prev_free) + KMEM_FREE_BLOCK_PTR(p_prev_free)->size == (uint8_t *)p_block) {
            // Merge with previous
            KMEM_FREE_BLOCK_PTR(p_prev_free)->size += block_size;
            // Account for reduced overhead when merging
            KMEM_FREE_BLOCK_PTR(p_prev_free)->size += sizeof(kmem_used_block_t);
        } else if ((uint8_t *)KMEM_FREE_BLOCK_PTR(p_prev_free->next) == (uint8_t *)p_block + block_size + sizeof(kmem_used_block_t)) {
            // Merge with next
            kmem_free_block_t *p_free = (void *)p_block;
            p_free->size = block_size + KMEM_FREE_BLOCK_PTR(p_prev_free->next)->size + sizeof(kmem_used_block_t);
            intrusive_list_init(&p_free->list_node);
            if (p_prev_free != p_prev_free->next) {
                // Account for reduced overhead when merging
                p_free->size += sizeof(kmem_used_block_t);
                intrusive_list_unlink_node(p_prev_free->next);
                intrusive_list_insert(p_prev_free, &p_free->list_node);
            } else {
                // No merging, replace single free block with new one
                heap_desc->free_list = &p_free->list_node;
            }
        } else {
            // Create new free block
            kmem_free_block_t *p_free = (void *)p_block;
            intrusive_list_init(&p_free->list_node);
            p_free->size = block_size;
            intrusive_list_insert(p_prev_free, &p_free->list_node);
        }
    }
}
