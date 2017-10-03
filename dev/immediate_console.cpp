#include "otrix/immediate_console.hpp"
#include "dev/serial.h"

namespace otrix
{

size_t strlen(const char *str)
{
    size_t size = 0;
    while (*str++) {
        size++;
    }

    return size;
}

// TODO: Move somewhere
static serial_dev immediate_com_port;

bool immediate_console::inited_ = false;

void immediate_console::init()
{
    if (inited_) {
        return;
    }

    serial_init(&immediate_com_port, SERIAL_BAUD_115200,
            SERIAL_COM1);

    print("\r\nImmediate console initialized\r\n");
    inited_ = true;
}

void immediate_console::print(const char *str)
{
    serial_write(&immediate_com_port,
            (const uint8_t *)str, strlen(str));
}

} // namespace otrix
