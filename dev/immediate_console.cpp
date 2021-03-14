#include "otrix/immediate_console.hpp"
#include "dev/serial.h"
#include <cstring>
#include <cstdarg>
#include <cstdio>
#include "arch/asm.h"

namespace otrix
{

// TODO: Move somewhere
extern "C" serial_dev immediate_com_port;
serial_dev immediate_com_port;

bool immediate_console::inited_;
char immediate_console::message_buffer_[immediate_console::buffer_size_];

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

void immediate_console::print(const char *format, ...)
{
    auto flags = arch_irq_save();
    va_list ap;
    va_start(ap, format);
    int size = vsnprintf(message_buffer_, buffer_size_, format, ap);
    va_end(ap);

    if (size < 0) {
        arch_irq_restore(flags);
        return;
    }

    serial_write(&immediate_com_port,
            (const uint8_t *)message_buffer_, size);
    arch_irq_restore(flags);
}

void immediate_console::write(const void *buf, size_t count)
{
    auto flags = arch_irq_save();
    serial_write(&immediate_com_port, (const uint8_t *)buf, count);
    arch_irq_restore(flags);
}

} // namespace otrix
