#include "kernel/kmem.hpp"

#include <cstring>
#include <cstdio>

void kmem_init(kmem_heap_t *heap_desc, void *heap_start, size_t heap_size)
{
    memset((void *)heap_desc, 0, sizeof(kmem_heap_t));
    heap_desc->start = heap_start;
    heap_desc->size = heap_size;

    kmem_free_block_t *first_free = (kmem_free_block_t *)heap_start;
    intrusive_list_init(&first_free->list_node);
    first_free->size = heap_size;
    first_free->magic = KMEM_BLOCK_MAGIC;
    heap_desc->free_list = &first_free->list_node;
}

void *kmem_alloc(kmem_heap_t *heap_desc, size_t size)
{
    size = KMEM_ALLOCATION_SIZE(size + sizeof(kmem_used_block_t));

    intrusive_list *p_free = heap_desc->free_list;
    if (nullptr == p_free) {
        return nullptr;
    }

    do {
        if (KMEM_FREE_BLOCK_PTR(p_free)->magic != KMEM_BLOCK_MAGIC) {
            // TODO: report heap corruption
        }
        if (size <= KMEM_FREE_BLOCK_PTR(p_free)->size) {
            break;
        }
        p_free = p_free->next;
    } while (p_free != heap_desc->free_list);

    if (KMEM_FREE_BLOCK_PTR(p_free)->size < size) {
        return nullptr;
    }

    if (p_free == heap_desc->free_list) {
        if (KMEM_FREE_BLOCK_PTR(p_free)->size - size < KMEM_MIN_FREE_BLOCK_SIZE) {
            heap_desc->free_list = intrusive_list_delete(heap_desc->free_list, p_free);
        } else {
            kmem_free_block_t *new_free = (kmem_free_block_t *)((uint8_t *)KMEM_FREE_BLOCK_PTR(p_free) + size);
            new_free->size = KMEM_FREE_BLOCK_PTR(p_free)->size - size;
            new_free->magic = KMEM_BLOCK_MAGIC;
            intrusive_list_init(&new_free->list_node);
            intrusive_list *prev = p_free->prev;
            if (prev != p_free) {
                intrusive_list_unlink_node(p_free);
                intrusive_list_insert(prev, &new_free->list_node);
            }
            heap_desc->free_list = &new_free->list_node;
        }
    } else {
        if (KMEM_FREE_BLOCK_PTR(p_free)->size - size < KMEM_MIN_FREE_BLOCK_SIZE) {
            heap_desc->free_list = intrusive_list_delete(heap_desc->free_list, p_free);
        } else {
            intrusive_list *prev = p_free->prev;
            kmem_free_block_t *new_free = (kmem_free_block_t *)((uint8_t *)KMEM_FREE_BLOCK_PTR(p_free) + size);
            intrusive_list_init(&new_free->list_node);
            new_free->size = KMEM_FREE_BLOCK_PTR(p_free)->size - size;
            new_free->magic = KMEM_BLOCK_MAGIC;
            intrusive_list_unlink_node(p_free);
            intrusive_list_insert(prev, &new_free->list_node);
        }
    }

    kmem_used_block_t *p_block = (kmem_used_block_t *)KMEM_FREE_BLOCK_PTR(p_free);
    p_block->magic = KMEM_BLOCK_MAGIC;
    p_block->size = size;

    return (void *)(p_block + 1);
}

void kmem_free(kmem_heap_t *heap_desc, void *ptr)
{
    if (nullptr == ptr) {
        // OK
        return;
    }

    kmem_used_block_t *p_block = (kmem_used_block_t *)ptr - 1;
    if (p_block < heap_desc->start || (uint8_t *)p_block > (uint8_t *)heap_desc->start + heap_desc->size) {
        // TODO: report bad free()
        return;
    }

    if (KMEM_BLOCK_MAGIC != p_block->magic) {
        // TODO: report bad free()
        return;
    }

    const size_t block_size = p_block->size;
    kmem_free_block_t *p_free = (kmem_free_block_t *)p_block;
    intrusive_list_init(&p_free->list_node);
    p_free->size = block_size;
    p_free->magic = KMEM_BLOCK_MAGIC;

    if (nullptr != heap_desc->free_list) {
        intrusive_list *p_head = heap_desc->free_list;
        intrusive_list *p_prev = nullptr;

        do {
            if ((void *)KMEM_FREE_BLOCK_PTR(p_head) > ptr) {
                break;
            }
            p_prev = p_head;
            p_head = p_head->next;
        } while (p_head != heap_desc->free_list);

        if (nullptr == p_prev) {
            heap_desc->free_list->prev->next = &p_free->list_node;
            p_free->list_node.prev = heap_desc->free_list->prev;
            p_free->list_node.next = heap_desc->free_list;
            heap_desc->free_list->prev = &p_free->list_node;
            heap_desc->free_list = &p_free->list_node;
        } else {
            intrusive_list_insert(p_prev, &p_free->list_node);
        }
    } else {
        heap_desc->free_list = &p_free->list_node;
    }

    kmem_free_block_t *p_prev_block = KMEM_FREE_BLOCK_PTR(p_free->list_node.prev);
    if ((uint8_t *)p_prev_block + p_prev_block->size == (uint8_t *)p_free) {
        // Merge with previous
        p_prev_block->size += block_size;
        intrusive_list_unlink_node(&p_free->list_node);
        p_free = p_prev_block;
    }

    kmem_free_block_t *p_next_block = KMEM_FREE_BLOCK_PTR(p_free->list_node.next);
    if ((uint8_t *)p_free + p_free->size == (uint8_t *)p_next_block) {
        // Merge with next
        p_free->size += p_next_block->size;
        intrusive_list_unlink_node(&p_next_block->list_node);
    }
}

void kmem_print_free(kmem_heap_t *heap_desc)
{
    intrusive_list *p_head = heap_desc->free_list;

    printf("Free blocks:\n");
    int max = 10;
    if (nullptr == p_head) {
        return;
    }
    do {
        const kmem_free_block_t *block = KMEM_FREE_BLOCK_PTR(p_head);
        printf("Free block %016lx %p (off %ld) size %lu\n", block->magic, block, (uint8_t *)block - (uint8_t *)heap_desc->start, block->size);
        p_head = p_head->next;
    } while (p_head != heap_desc->free_list && max-- > 0);
}
