#include <kernel/kmem.h>
#include <string.h>
#include <unity.h>
#include <unity_fixture.h>

TEST_GROUP(kmem_tests);

TEST_SETUP(kmem_tests)
{

}

TEST_TEAR_DOWN(kmem_tests)
{

}

static void test_empty_heap(kmem_heap_t *heap_desc, void *heap, size_t heap_size) {
    TEST_ASSERT_EQUAL_PTR(heap, heap_desc->start);
    TEST_ASSERT_EQUAL(heap_size, heap_desc->size);
    TEST_ASSERT_EQUAL_PTR(KMEM_FREE_BLOCK_PTR(heap_desc->free_list), heap_desc->start);
    kmem_free_block_t *first_free = heap;
    TEST_ASSERT_EQUAL(heap_size - sizeof(kmem_used_block_t), first_free->size);
    TEST_ASSERT_EQUAL_PTR(&first_free->list_node, first_free->list_node.next);
    TEST_ASSERT_EQUAL_PTR(&first_free->list_node, first_free->list_node.prev);
}

TEST(kmem_tests, kmem_init)
{
    const size_t heap_size = 1024;
    void *heap = malloc(heap_size);
    TEST_ASSERT_NOT_EQUAL(NULL, heap);

    kmem_heap_t heap_desc;
    kmem_init(&heap_desc, heap, heap_size);
    test_empty_heap(&heap_desc, heap, heap_size);

    free(heap);
}

TEST(kmem_tests, kmem_single_alloc_free)
{
    const int heap_size = 1024;
    void *heap = malloc(heap_size);
    TEST_ASSERT_NOT_EQUAL(NULL, heap);

    kmem_heap_t heap_desc;
    kmem_init(&heap_desc, heap, heap_size);

    for (size_t alloc_size = 1; alloc_size < heap_size - sizeof(kmem_used_block_t); alloc_size += heap_size / 10) {
        void *mem = kmem_alloc(&heap_desc, alloc_size);
        memset(mem, 0xFF, alloc_size);
        const size_t padded_alloc_size = KMEM_PAD_ALLOCATION_SIZE(alloc_size);
        TEST_ASSERT_EQUAL((uint8_t *)heap_desc.start + sizeof(kmem_used_block_t), mem);
        const kmem_used_block_t *p_block = (kmem_used_block_t *)mem - 1;
        TEST_ASSERT_EQUAL_HEX(KMEM_BLOCK_MAGIC, p_block->magic);
        TEST_ASSERT_EQUAL_HEX(padded_alloc_size, p_block->size);
        const kmem_free_block_t *p_free_block = (void *)((uint8_t *)heap + padded_alloc_size + sizeof(kmem_used_block_t));
        TEST_ASSERT_EQUAL_PTR(p_free_block, KMEM_FREE_BLOCK_PTR(heap_desc.free_list));
        TEST_ASSERT_EQUAL(heap_size - padded_alloc_size - sizeof(kmem_used_block_t) * 2, p_free_block->size);
        kmem_free(&heap_desc, mem);
        test_empty_heap(&heap_desc, heap, heap_size);
    }

    free(heap);
}

TEST(kmem_tests, kmem_triple_alloc_free)
{
    const int heap_size = 1024;
    void *heap = malloc(heap_size);
    TEST_ASSERT_NOT_EQUAL(NULL, heap);

    kmem_heap_t heap_desc;
    kmem_init(&heap_desc, heap, heap_size);

    const size_t alloc_size = 256;
    void *a = kmem_alloc(&heap_desc, alloc_size);
    TEST_ASSERT_NOT_EQUAL(NULL, a);
    void *b = kmem_alloc(&heap_desc, alloc_size);
    TEST_ASSERT_NOT_EQUAL(NULL, b);
    void *c = kmem_alloc(&heap_desc, alloc_size);
    TEST_ASSERT_NOT_EQUAL(NULL, c);

    kmem_free_block_t *p_free = KMEM_FREE_BLOCK_PTR(heap_desc.free_list);
    // 3 allocations padded with padding + 3 kmem_used_block_t + 1 kmem_used_block_t accounted in free block size
    const size_t used_size = KMEM_PAD_ALLOCATION_SIZE(alloc_size) * 3 + sizeof(kmem_used_block_t) * 3;
    TEST_ASSERT_EQUAL_INT(heap_size - used_size, p_free->size + sizeof(kmem_used_block_t));
    // Only single free block should be present
    TEST_ASSERT_EQUAL_PTR(p_free->list_node.next, &p_free->list_node);
    TEST_ASSERT_EQUAL_PTR(p_free->list_node.prev, &p_free->list_node);
    TEST_ASSERT_EQUAL_PTR((uint8_t *)heap + used_size, p_free);

    kmem_free(&heap_desc, a);
    p_free = KMEM_FREE_BLOCK_PTR(heap_desc.free_list);
    // Should be two free blocks now
    // First block at the start
    TEST_ASSERT_EQUAL_PTR(heap, p_free);
    kmem_free_block_t *p_second_free = KMEM_FREE_BLOCK_PTR(p_free->list_node.next);
    // Check that the list has two elements
    TEST_ASSERT_EQUAL_PTR(&p_free->list_node, p_second_free->list_node.next);
    TEST_ASSERT_EQUAL_PTR(&p_free->list_node, p_second_free->list_node.prev);

    TEST_ASSERT_EQUAL_INT(KMEM_PAD_ALLOCATION_SIZE(alloc_size), p_free->size);
    // Check that second block size has not changed since last time
    TEST_ASSERT_EQUAL_INT(heap_size - used_size, p_second_free->size + sizeof(kmem_used_block_t));

    kmem_free(&heap_desc, c);
    p_free = KMEM_FREE_BLOCK_PTR(heap_desc.free_list);
    // There should be two free blocks (the last two were merged)
    // First block at the start
    TEST_ASSERT_EQUAL_PTR(heap, p_free);
    p_second_free = KMEM_FREE_BLOCK_PTR(p_free->list_node.next);
    // Check that the list has two elements
    TEST_ASSERT_EQUAL_PTR(&p_free->list_node, p_second_free->list_node.next);
    TEST_ASSERT_EQUAL_PTR(&p_free->list_node, p_second_free->list_node.prev);

    TEST_ASSERT_EQUAL_INT(KMEM_PAD_ALLOCATION_SIZE(alloc_size), p_free->size);
    // Check that second block size has not changed since last time
    TEST_ASSERT_EQUAL_INT(heap_size - 2 * KMEM_PAD_ALLOCATION_SIZE(alloc_size), p_second_free->size + 3 * sizeof(kmem_used_block_t));

    kmem_free(&heap_desc, b);
    test_empty_heap(&heap_desc, heap, heap_size);

    free(heap);
}

TEST_GROUP_RUNNER(kmem_tests)
{
    RUN_TEST_CASE(kmem_tests, kmem_init);
    RUN_TEST_CASE(kmem_tests, kmem_single_alloc_free);
    RUN_TEST_CASE(kmem_tests, kmem_triple_alloc_free);
}
