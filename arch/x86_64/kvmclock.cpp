#include "arch/kvmclock.hpp"
#include "arch/asm.h"
#include "otrix/immediate_console.hpp"
#include <cstring>

#define KVM_CPUID_SIGNATURE 0x40000000
#define KVM_FEATURE_CLOCKSOURCE2 3
#define MSR_KVM_WALL_CLOCK_NEW 0x4b564d00
#define MSR_KVM_SYSTEM_TIME_NEW 0x4b564d01

namespace otrix::arch::kvmclock
{

struct alignas(4096) pvclock_wall_clock {
    uint32_t version;
    uint32_t sec;
    uint32_t nsec;
} __attribute__((__packed__));

struct alignas(4096) pvclock_vcpu_time_info {
	uint32_t version;
	uint32_t pad0;
	uint64_t tsc_timestamp;
	uint64_t system_time;
	uint32_t tsc_to_system_mul;
	signed char tsc_shift;
	unsigned char flags;
	unsigned char pad[2];
}__attribute__((packed));

static const volatile pvclock_wall_clock wall_clock = {0, 0, 0};
static pvclock_vcpu_time_info time_info;

bool init()
{
    uint32_t eax, ebx, ecx, edx;
    arch_cpuid(KVM_CPUID_SIGNATURE, 0, &eax, &ebx, &ecx, &edx);
    immediate_console::print("KVM signature: %08x %08x %08x %08x\n", eax, ebx, ecx, edx);
    if (ebx != 0x4b4d564b || ecx != 0x564b4d56 || edx != 0x4d) {
        return false;
    }
    const uint32_t function = eax;
    arch_cpuid(function, 0, &eax, &ebx, &ecx, &edx);
    immediate_console::print("KVM features: %08x\n", eax);
    if (0 == (eax & (1 << KVM_FEATURE_CLOCKSOURCE2))) {
        return false;
    }
    arch_write_msr(MSR_KVM_WALL_CLOCK_NEW, reinterpret_cast<uint64_t>(&wall_clock));
    arch_write_msr(MSR_KVM_SYSTEM_TIME_NEW, reinterpret_cast<uint64_t>(&time_info) | 1);
    immediate_console::print("%08lx %p\n", arch_read_msr(MSR_KVM_WALL_CLOCK_NEW), &wall_clock);
    return true;
}

uint64_t systime_ms()
{
    uint32_t version;
    uint32_t sec, nsec;
    arch_write_msr(MSR_KVM_WALL_CLOCK_NEW, reinterpret_cast<uint64_t>(&wall_clock));
	do {
		version = wall_clock.version;
		asm("mfence" ::: "memory");
		sec = wall_clock.sec;
		nsec = wall_clock.nsec;
		asm("mfence" ::: "memory");
	} while ((wall_clock.version & 1) || (wall_clock.version != version));
    return sec * 1000 + nsec / 1000000;
}

} // namespace otrix::arch::kvmclock
