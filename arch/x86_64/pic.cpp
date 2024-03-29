#include <arch/pic.hpp>
#include <arch/asm.h>

#define PIC1_COMMAND  0x20
#define PIC2_COMMAND  0xa0
#define PIC1_DATA     0x21
#define PIC2_DATA     0xa1

#define ICW1_ICW4	0x01		/* ICW4 (not) needed */
#define ICW1_SINGLE	0x02		/* Single (cascade) mode */
#define ICW1_INTERVAL4	0x04		/* Call address interval 4 (8) */
#define ICW1_LEVEL	0x08		/* Level triggered (edge) mode */
#define ICW1_INIT	0x10		/* Initialization - required! */

#define ICW4_8086	0x01		/* 8086/88 (MCS-80/85) mode */
#define ICW4_AUTO	0x02		/* Auto (normal) EOI */
#define ICW4_BUF_SLAVE	0x08		/* Buffered mode/slave */
#define ICW4_BUF_MASTER	0x0C		/* Buffered mode/master */
#define ICW4_SFNM	0x10		/* Special fully nested (not) */

namespace otrix::arch
{

void pic_init(int offset1, int offset2)
{
	unsigned char a1, a2;

	a1 = arch_io_read8(PIC1_DATA);                        // save masks
	a2 = arch_io_read8(PIC2_DATA);

	arch_io_write8(PIC1_COMMAND, ICW1_INIT | ICW1_ICW4);  // starts the initialization sequence (in cascade mode)
	arch_io_wait();
	arch_io_write8(PIC2_COMMAND, ICW1_INIT | ICW1_ICW4);
	arch_io_wait();
	arch_io_write8(PIC1_DATA, offset1);                 // ICW2: Master PIC vector offset
	arch_io_wait();
	arch_io_write8(PIC2_DATA, offset2);                 // ICW2: Slave PIC vector offset
	arch_io_wait();
	arch_io_write8(PIC1_DATA, 4);                       // ICW3: tell Master PIC that there is a slave PIC at IRQ2 (0000 0100)
	arch_io_wait();
	arch_io_write8(PIC2_DATA, 2);                       // ICW3: tell Slave PIC its cascade identity (0000 0010)
	arch_io_wait();

	arch_io_write8(PIC1_DATA, ICW4_8086);
	arch_io_wait();
	arch_io_write8(PIC2_DATA, ICW4_8086);
	arch_io_wait();

	arch_io_write8(PIC1_DATA, a1);   // restore saved masks.
	arch_io_write8(PIC2_DATA, a2);
}

void pic_disable(void)
{
    arch_io_write8(0x21, 0xff);
    arch_io_write8(0xa1, 0xff);
}

} // namespace otrix::arch
