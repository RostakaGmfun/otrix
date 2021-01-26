#include "kernel/kmem.h"

void kmem_init(kmem_heap_t *heap_desc, void *heap_start, size_t heap_size) {
    memset(heap_desc, 0, sizeof(kmem_heap_t));
}

void *kmem_alloc(size_t size) {

}

void *kmem_free(void *ptr) {

}
