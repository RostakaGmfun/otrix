#include "serial.h"
#include <stdbool.h>

#include "arch/io.h"

struct serial_dev
{
    serial_com_port com_port;
    serial_baud_rate baud;
};

enum serial_registers
{
    SERIAL_DATA, // Budrate divider low byte when DLAB == 1
    SERIAL_INTEN, // Budrate divider high byte when DLAB == 1
    SERIAL_IIR_FCR, // IIR on read, FCR on write
    SERIAL_LINE_CTL,
    SERIAL_MODEM_CTL,
    SERIAL_LSR,
    SERIAL_MODEM_STATUS,
    SERIAL_SCRATCH_REG,
};

#define SERIAL_LINE_CTL_DLAB (1 << 8)
#define SERIAL_LINE_CTL_8BIT (3 << 1)
#define SERIAL_LINE_CTL_1STOP_BIT (0 << 2)
#define SERIAL_LINE_CTL_NO_PARITY (0 << 3)

#define SERIAL_FCR_ENABLE_FIFO (1 << 0)
#define SERIAL_FCR_CLEAR_RX_FIFO (1 << 1)
#define SERIAL_FCR_CLEAR_TX_FIFO (1 << 2)
#define SERIAL_FCR_14BYTES_FIFO 0xC0

#define SERIAL_LSR_RX_READY (1 << 0)
#define SERIAL_LSR_TX_EMPTY (1 << 5)

static void serial_set_baud(const struct serial_dev *self,
        const enum serial_baud_rate baud)
{
    // Enable DLAB
    uint8_t line_ctl = arch_io_read_byte(self->com_port + SERIAL_LINE_CTL);
    line_ctl |= SERIAL_LINE_CTL_DLAB;
    arch_io_write_byte(SERIAL_LINE_CTL, line_ctl);

    arch_io_write_byte(self->com_port + SERIAL_DATA, (uint16_t)baud & 0xFF);
    arch_io_write_byte(self->com_port + SERIAL_INTEN, (uint16_t)baud & 0xFF00);

    // Disable DLAB
    line_ctl &= ~SERIAL_LINE_CTL_DLAB;
    arch_io_write_byte(self->com_port + SERIAL_LINE_CTL, line_ctl);
}

static bool serial_tx_ready(const struct serial_dev * const self)
{
    return arch_io_read_byte(self->com_port + SERIAL_LSR) & SERIAL_LSR_TX_EMPTY;
}

static bool serial_rx_ready(const struct serial_dev * const self)
{
    return arch_io_read_byte(self->com_port + SERIAL_LSR) & SERIAL_LSR_RX_READY;
}

void serial_init(struct serial_dev *serial,
        const enum serial_baud_rate baud, const serial_com_port com_port)
{
    serial->com_port = com_port;
    serial->baud = baud;

    arch_io_write_byte(self->com_port + SERIAL_DATA, 0x00); // Disable all interrupts
    serial_set_baud(self, baud);
    arch_io_write_byte(self->com_port, SERIAL_LINE_CTL_8BIT |
            SERIAL_LINE_CTL_1STOP_BIT | SERIAL_LINE_CTL_NO_PARITY);
    arch_io_write_byte(SERIAL_IIR_FCR,
            SERIAL_FCR_ENABLE_FIFO | SERIAL_FCR_CLEAR_RX_FIFO |
            SERIAL_FCR_CLEAR_TX_FIFO | SERIAL_FCR_14BYTES_FIFO);
    arch_io_write_byte(SERIAL_MODEM_CTL, 0x00);
}

void serial_write(const uint8_t * const data, const size_t data_size)
{
    for (size_t i = 0; i < data_size; i++) {
        while (!serial_tx_ready(self));
        arch_io_write_byte(self->com_port + SERIAL_DATA, data[i]);
    }
}

void serial_read(uint8_t * const buffer, const size_t buffer_size)
{
    for (size_t i = 0; i < buffer_size; i++) {
        while (!serial_rx_ready(self));
        buffer[i] = arch_io_read_byte(self->com_port + SERIAL_DATA);
    }
}
