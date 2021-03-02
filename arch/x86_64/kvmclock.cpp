#include "arch/kvmclock.hpp"
#include "arch/asm.h"
#include <cstring>
#include "otrix/immediate_console.hpp"

#define KVM_CPUID_SIGNATURE 0x40000000
#define KVM_FEATURE_CLOCKSOURCE2 3
#define MSR_KVM_WALL_CLOCK_NEW 0x4b564d00
#define MSR_KVM_SYSTEM_TIME_NEW 0x4b564d01

namespace otrix::arch::kvmclock
{

struct pvclock_wall_clock {
    uint32_t version;
    uint32_t sec;
    uint32_t nsec;
} __attribute__((__packed__));

struct pvclock_vcpu_time_info {
    uint32_t version;
    uint32_t pad0;
    uint64_t tsc_timestamp;
    uint64_t system_time;
    uint32_t tsc_to_system_mul;
    signed char tsc_shift;
    unsigned char flags;
    unsigned char pad[2];
}__attribute__((packed));

static volatile pvclock_wall_clock wall_clock;
static volatile pvclock_vcpu_time_info time_info;

bool init()
{
    uint32_t eax, ebx, ecx, edx;
    arch_cpuid(KVM_CPUID_SIGNATURE, 0, &eax, &ebx, &ecx, &edx);
    if (ebx != 0x4b4d564b || ecx != 0x564b4d56 || edx != 0x4d) {
        return false;
    }
    const uint32_t function = eax;
    arch_cpuid(function, 0, &eax, &ebx, &ecx, &edx);
    if (0 == (eax & (1 << KVM_FEATURE_CLOCKSOURCE2))) {
        return false;
    }
    arch_write_msr(MSR_KVM_WALL_CLOCK_NEW, reinterpret_cast<uint64_t>(&wall_clock));
    arch_write_msr(MSR_KVM_SYSTEM_TIME_NEW, reinterpret_cast<uint64_t>(&time_info) | 1);
    return true;
}

uint64_t boottime_ns()
{
    uint32_t version;
    uint32_t sec, nsec;
    arch_write_msr(MSR_KVM_WALL_CLOCK_NEW, reinterpret_cast<uint64_t>(&wall_clock));
    do {
        version = wall_clock.version;
        asm("lfence" ::: "memory");
        sec = wall_clock.sec;
        nsec = wall_clock.nsec;
        asm("lfence" ::: "memory");
    } while ((wall_clock.version & 1) || (wall_clock.version != version));
    return sec * 1000 + nsec / 1000000;
}

uint64_t ns_to_tsc(uint64_t ns)
{
    uint32_t version;
    uint64_t tsc_to_system_mul;
    signed char tsc_shift;
    do {
        version = time_info.version;
        asm("lfence" ::: "memory");
        tsc_to_system_mul = time_info.tsc_to_system_mul;
        tsc_shift = time_info.tsc_shift;
        asm("lfence" ::: "memory");
    } while ((time_info.version & 1) || (time_info.version != version));

    uint64_t tmp = 0;
    asm volatile("shld $32, %%rax, %%rdx; shl $32, %%rax; divq %[mul_fraq]"
            : "=a"(ns): "a"(ns), "d"(tmp), [mul_fraq]"rm"(tsc_to_system_mul));

    if (tsc_shift < 0) {
        ns <<= -tsc_shift;
    } else {
        ns >>= tsc_shift;
    }
    return ns;
}

uint64_t systime_ns()
{
    uint32_t version;
    uint64_t tsc_timestamp;
    uint64_t system_time;
    uint64_t tsc_to_system_mul;
    signed char tsc_shift;
    do {
        version = time_info.version;
        asm("lfence" ::: "memory");
        tsc_timestamp = time_info.tsc_timestamp;
        system_time = time_info.system_time;
        tsc_to_system_mul = time_info.tsc_to_system_mul;
        tsc_shift = time_info.tsc_shift;
        asm("lfence" ::: "memory");
    } while ((time_info.version & 1) || (time_info.version != version));

    const uint64_t current_tsc = arch_tsc();
    uint64_t time = (current_tsc - tsc_timestamp);
    if (tsc_shift < 0) {
        time >>= -tsc_shift;
    } else {
        time <<= tsc_shift;
    }
    uint64_t tmp;
    // Taken from Linux.
    // For some reason using GCC generates imul instruction which gives result mod 2^32
    asm volatile (
        "mulq %[mul_frac] ; shrd $32, %[hi], %[lo]"
        : [lo]"=a" (time), [hi]"=d"(tmp)
        : "0" (time), [mul_frac]"rm"((uint64_t)tsc_to_system_mul));

    time += system_time;
    return time;
}

uint64_t systime_us()
{
    return systime_ns() / 1000;
}

uint64_t systime_ms()
{
    return systime_ns() / 1000000;
}

} // namespace otrix::arch::kvmclock
