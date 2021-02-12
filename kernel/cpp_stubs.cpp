extern "C"
{

extern void (*__preinit_array_start[])() __attribute__((weak));
extern void (*__preinit_array_end[])() __attribute__((weak));
extern void (*__init_array_start[])() __attribute__((weak));
extern void (*__init_array_end[])() __attribute__((weak));

void __arch_init_array()
{
    const int preinit_length = __preinit_array_end - __preinit_array_start;
    for (int i = 0; i < preinit_length; i++) {
        __preinit_array_start[i]();
    }

    const int init_length = __init_array_end - __init_array_start;
    for (int i = 0; i < init_length; i++) {
        __init_array_start[i]();
    }
}

void *__dso_handle;

int __cxa_atexit(void (*destructor) (void *), void *arg, void *dso)
{
    (void)destructor;
    (void)arg;
    (void)dso;
    return 0;
}

void __cxa_finalize(void *f)
{
    (void)f;
}

}
