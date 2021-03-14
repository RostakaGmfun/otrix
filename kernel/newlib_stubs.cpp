#include "common/assert.h"
#include "otrix/immediate_console.hpp"

extern "C" {

int write(int fd, const char *buf, int count)
{
    if (fd == 1 || fd == 2) {
        otrix::immediate_console::write(buf, count);
        return count;
    }
    return -1;
}

int read()
{
    kASSERT(0);
}

int close()
{
    kASSERT(0);
}

int isatty()
{
    kASSERT(0);
}

int lseek()
{
    kASSERT(0);
}

int fstat()
{
    return 0;
}

int sbrk()
{
    return 0;
}

int kill()
{
    kASSERT(0);
}

void _exit(int arg)
{
    (void)arg;
    kASSERT(0);
}

int getpid()
{
    kASSERT(0);
}

}
