#include "serial.h"

void kmain()
{
    struct serial_dev serial;
    serial_init(&serial, SERIAL_BAUD_115200, SERIAL_COM1);
    const char str[] = "Hello, otrix!\n";
    serial_write(&serial, str, sizeof(str) - 1);
    while (1);
}
