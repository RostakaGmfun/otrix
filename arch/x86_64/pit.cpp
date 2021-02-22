#include "arch/pit.hpp"
#include "arch/asm.h"
#include "arch/irq_manager.hpp"

namespace otrix::arch::pit
{

enum Mode { ONE_SHOT = 0,
            HW_ONESHOT = 1 << 1,
            RATE_GEN = 2 << 1,
            SQ_WAVE = 3 << 1,
            SW_STROBE = 4 << 1,
            HW_STROBE = 5 << 1,
            NONE = 256};

enum AccessMode { LATCH_COUNT = 0x0, LO_ONLY=0x10, HI_ONLY=0x20, LO_HI=0x30 };

static constexpr auto one_ms = 1194; // 1ms in (14.31818 / 12) MHz ticks

void start(int ticks)
{
    const uint8_t config = LO_HI | ONE_SHOT;
    arch_io_write8(0x43, config);
    const uint16_t count = ticks;
    arch_io_write8(0x40, count & 0xff);
    arch_io_write8(0x40, count >> 8);
}

int get_ticks()
{
    arch_io_write8(0x43, 0);
    uint16_t ret = arch_io_read8(0x40) << 8;
    ret |= arch_io_read8(0x40);
    return ret;
}

} // namespace otrix::arch::pit
