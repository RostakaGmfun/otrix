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

TEST(kmem_tests, kmem_init)
{
    const int heap_size = 1024;
    void *heap = malloc(heap_size);
    TEST_ASSERT_NOT_EQUAL(NULL, heap);

    kmem_heap_t heap_desc;
    kmem_init(&heap_desc, heap, heap_size);
    TEST_ASSERT_EQUAL_PTR(heap, heap_desc.start);
    TEST_ASSERT_EQUAL(heap_size, heap_desc.size);

    free(heap);
}

TEST_GROUP_RUNNER(kmem_tests)
{
    RUN_TEST_CASE(kmem_tests, kmem_init);
}
