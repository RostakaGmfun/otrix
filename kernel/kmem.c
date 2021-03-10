#include "kernel/kmem.h"

#include <string.h>

#if defined(BUILD_HOST_TESTS)
#include <stdio.h>
#endif

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

    size = KMEM_PAD_ALLOCATION_SIZE(size);

    struct intrusive_list *p_free = heap_desc->free_list;
    do {
        if (size <= KMEM_FREE_BLOCK_PTR(p_free)->size) {
            break;
        }
        p_free = p_free->next;
    } while (p_free != heap_desc->free_list);

    if (KMEM_FREE_BLOCK_PTR(p_free)->size < size) {
        return NULL;
    }

    kmem_used_block_t *p_block = (void *)KMEM_FREE_BLOCK_PTR(p_free);
    uint8_t *p_after = (uint8_t *)p_block + sizeof(kmem_used_block_t) + size;

    if (p_free == heap_desc->free_list) {
        if ((uint8_t *)heap_desc->start + heap_desc->size - p_after < (long int)sizeof(kmem_free_block_t)) {
            heap_desc->free_list = NULL;
        } else {
            kmem_free_block_t *new_free = (kmem_free_block_t *)p_after;
            new_free->size = KMEM_FREE_BLOCK_PTR(p_free)->size - size - sizeof(kmem_used_block_t);
            intrusive_list_init(&new_free->list_node);
            heap_desc->free_list = &new_free->list_node;
        }
    } else {
        // Sufficient space to allocate another free block?
        if ((uint8_t *)heap_desc->start + heap_desc->size - p_after > (long int)sizeof(kmem_free_block_t)) {
            struct intrusive_list *prev = prev = p_free->prev;
            kmem_free_block_t *new_free = (kmem_free_block_t *)p_after;
            intrusive_list_init(&new_free->list_node);
            new_free->size = KMEM_FREE_BLOCK_PTR(p_free)->size - size - sizeof(kmem_used_block_t);
            intrusive_list_unlink_node(p_free);
            intrusive_list_insert(prev, &new_free->list_node);
        } else {
            intrusive_list_unlink_node(p_free);
        }
    }

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
    // Create new free block
    kmem_free_block_t *p_free = (void *)p_block;
    intrusive_list_init(&p_free->list_node);
    p_free->size = block_size;

    struct intrusive_list *p_head = heap_desc->free_list;
    struct intrusive_list *p_prev = NULL;
    do {
        if ((void *)KMEM_FREE_BLOCK_PTR(p_head) > ptr) {
            break;
        }
        p_prev = p_head;
        p_head = p_head->next;
    } while (p_head != heap_desc->free_list);

    if (NULL == p_prev) {
        heap_desc->free_list->prev->next = &p_free->list_node;
        p_free->list_node.prev = heap_desc->free_list->prev;
        p_free->list_node.next = heap_desc->free_list;
        heap_desc->free_list->prev = &p_free->list_node;
        heap_desc->free_list = &p_free->list_node;
    } else {
        p_free->list_node.prev = p_prev;
        p_free->list_node.next = p_prev->next;
        p_prev->next = &p_free->list_node;
        p_free->list_node.next->prev = &p_free->list_node;
    }

    kmem_free_block_t *p_prev_block = KMEM_FREE_BLOCK_PTR(p_free->list_node.prev);
    if ((uint8_t *)p_prev_block + p_prev_block->size + sizeof(kmem_used_block_t) == (uint8_t *)p_free) {
        // Merge with previous
        p_prev_block->size += block_size + sizeof(kmem_used_block_t);
        intrusive_list_unlink_node(&p_free->list_node);
        p_free = p_prev_block;
    }

    kmem_free_block_t *p_next_block = KMEM_FREE_BLOCK_PTR(p_free->list_node.next);
    if ((uint8_t *)p_free + p_free->size + sizeof(kmem_used_block_t) == (uint8_t *)p_next_block) {
        // Merge with next
        p_free->size += p_next_block->size + sizeof(kmem_used_block_t);
        intrusive_list_unlink_node(&p_next_block->list_node);
    }

}

#if defined(BUILD_HOST_TESTS)
void kmem_print_free(kmem_heap_t *heap_desc) {
    struct intrusive_list *p_head = heap_desc->free_list;

    int max = 10;
    do {
        const kmem_free_block_t *block = KMEM_FREE_BLOCK_PTR(p_head);
        printf("Free block %p (off %08d) size %zd\n", block, (uint8_t *)block - (uint8_t *)heap_desc->start, block->size + sizeof(kmem_used_block_t));
        p_head = p_head->next;
    } while (p_head != heap_desc->free_list && max-- > 0);
}
#endif
