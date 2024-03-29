#ifndef DEV_SERIAL_H
#define DEV_SERIAL_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

enum serial_baud_rate
{
    SERIAL_BAUD_115200 = 0x0001,
};

enum serial_com_port
{
    SERIAL_COM1 = 0x3F8,
    SERIAL_COM2 = 0x2F8,
    SERIAL_COM3 = 0x3E8,
    SERIAL_COM4 = 0x2E8,
};

struct serial_dev
{
    enum serial_baud_rate baud;
    enum serial_com_port com_port;
};

/**
 * Initialize serial device
 */
void serial_init(struct serial_dev *serial,
        const enum serial_baud_rate baud, const enum serial_com_port com_port);

/**
 * Blocking write @c data_size bytes to serial.
 */
void serial_write(const struct serial_dev *self,
        const uint8_t * const data, const size_t data_size);

/**
 * Blocking read exactly @c buffer_size from serial.
 */
void serial_read(const struct serial_dev *self,
        uint8_t * const buffer, const size_t buffer_size);

/**
 * Blocking get single byte from serial device.
 */
static inline uint8_t serial_get(const struct serial_dev *self)
{
    uint8_t byte;
    serial_read(self, &byte, sizeof(byte));
    return byte;
}

/**
 * Blocking write single byte to serial device;
 */
static inline void serial_put(const struct serial_dev *self,
        const uint8_t byte)
{
    serial_write(self, &byte, sizeof(byte));
}

#ifdef __cplusplus
}
#endif

#endif // DEV_SERIAL_H
