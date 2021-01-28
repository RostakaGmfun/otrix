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

    {
        const size_t alloc_size = 16;
        void *mem = kmem_alloc(&heap_desc, alloc_size);
        TEST_ASSERT_EQUAL((uint8_t *)heap_desc.start + sizeof(kmem_used_block_t), mem);
        const kmem_used_block_t *p_block = (kmem_used_block_t *)mem - 1;
        TEST_ASSERT_EQUAL_HEX(KMEM_BLOCK_MAGIC, p_block->magic);
        TEST_ASSERT_EQUAL_HEX(alloc_size, p_block->size);
        const kmem_free_block_t *p_free_block = (void *)((uint8_t *)heap + alloc_size + sizeof(kmem_used_block_t));
        TEST_ASSERT_EQUAL(p_free_block, KMEM_FREE_BLOCK_PTR(heap_desc.free_list));
        TEST_ASSERT_EQUAL(heap_size - alloc_size - sizeof(kmem_used_block_t) * 2, p_free_block->size);
        kmem_free(&heap_desc, mem);
        test_empty_heap(&heap_desc, heap, heap_size);
    }

    free(heap);
}

TEST_GROUP_RUNNER(kmem_tests)
{
    RUN_TEST_CASE(kmem_tests, kmem_init);
    RUN_TEST_CASE(kmem_tests, kmem_single_alloc_free);
}
