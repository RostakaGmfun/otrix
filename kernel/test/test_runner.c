#include <unity_fixture.h>

static void run_tests(void)
{
    RUN_TEST_GROUP(kmem_tests);
}

int main(int argc, const char **argv)
{
    return UnityMain(argc, argv, run_tests);
}
