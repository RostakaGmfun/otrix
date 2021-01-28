#include <common/list.h>
#include <string.h>
#include <unity.h>
#include <unity_fixture.h>

TEST_GROUP(list_tests);

TEST_SETUP(list_tests)
{

}

TEST_TEAR_DOWN(list_tests)
{

}

typedef struct test_data {
    struct intrusive_list node;
    int data;
} test_data_t;

TEST(list_tests, test_list_push_back)
{
    test_data_t test_data = { 0 };
    test_data.data = 42;
    intrusive_list_init(&test_data.node);

    test_data_t test_data2 = { 0 };
    test_data2.data = 13;
    intrusive_list_init(&test_data2.node);

    struct intrusive_list *tail = intrusive_list_push_back(&test_data.node,
            &test_data2.node);
    TEST_ASSERT_EQUAL_PTR(tail, intrusive_list_get_tail(&test_data.node));
    TEST_ASSERT_EQUAL_PTR(&test_data.node, test_data2.node.next);
}

static bool mock_predicate(const struct intrusive_list *node,
        void *context)
{
    return container_of(node, test_data_t, node)->data
        == *(int*)context;
}

TEST(list_tests, test_list_delete_first)
{
    test_data_t test_data[10];
    memset(test_data, 0, sizeof(test_data));
    struct intrusive_list *head = &(test_data[0].node);
    for (int i = 0; i < sizeof(test_data)/sizeof(test_data_t); i++) {
        intrusive_list_init(&test_data[i].node);
        if (&test_data[i].node != head) {
            intrusive_list_push_back(head, &(test_data[i].node));
        }
        test_data[i].data = i;
    }

    int id = 2;
    head = intrusive_list_delete_first(head, mock_predicate, &id);
    TEST_ASSERT_EQUAL_PTR(head, &test_data[0].node);

    id = 0;
    head = intrusive_list_delete_first(head, mock_predicate, &id);
    TEST_ASSERT_EQUAL_PTR(head, &test_data[1].node);

    TEST_ASSERT_EQUAL_PTR(&test_data[9].node,
            intrusive_list_get_tail(head));
}

TEST(list_tests, test_list_insert)
{
    test_data_t tdt1 = { .node = { 0 }, .data = 1 };
    test_data_t tdt2 = { .node = { 0 }, .data = 2 };
    intrusive_list_init(&tdt1.node);
    intrusive_list_init(&tdt2.node);

    test_data_t to_insert = { .node = { 0 }, .data = 42 };
    intrusive_list_init(&to_insert.node);

    intrusive_list_push_back(&tdt1.node, &tdt2.node);

    intrusive_list_insert(&tdt1.node, &to_insert.node);
    TEST_ASSERT_EQUAL_PTR(tdt1.node.next, &to_insert.node);
    TEST_ASSERT_EQUAL_PTR(tdt2.node.prev, &to_insert.node);
}

TEST_GROUP_RUNNER(list_tests)
{
    RUN_TEST_CASE(list_tests, test_list_push_back);
    RUN_TEST_CASE(list_tests, test_list_delete_first);
    RUN_TEST_CASE(list_tests, test_list_insert);
}
