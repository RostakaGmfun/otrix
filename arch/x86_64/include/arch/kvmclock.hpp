#pragma once

#include <cstdint>

namespace otrix::arch::kvmclock
{

bool init();

uint64_t boottime_ns();
uint64_t systime_ns();
uint64_t systime_us();
uint64_t systime_ms();

} // namespace otrix::arch::kvmclock
