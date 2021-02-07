#include "arch/paging.hpp"

#include <cstdint>
#include <type_traits>

namespace otrix::arch
{

static constexpr auto page_table_alignment = 4096;

alignas(page_table_alignment) static uint64_t p4_table[512];
alignas(page_table_alignment) static uint64_t p3_table[512];
alignas(page_table_alignment) static uint64_t p2_tables[4][512];

constexpr auto PAGE_PRESENT  = 1LU << 0;
constexpr auto PAGE_RW       = 1LU << 1;
constexpr auto PAGE_WT       = 1LU << 3;
constexpr auto PAGE_CD       = 1LU << 4;
constexpr auto PAGE_HUGE     = 1LU << 7;
constexpr auto PAGE_PAT_HUGE = 1LU << 12;

static uint64_t make_p4_entry(const uint64_t p3_addr)
{
    return PAGE_PRESENT | PAGE_RW | p3_addr;
}

static uint64_t make_p3_entry(const uint64_t p2_addr)
{
    return PAGE_PRESENT | PAGE_RW | p2_addr;
}

static uint64_t make_p3_entry_huge(const uint64_t phys_addr)
{
    return PAGE_PRESENT | PAGE_RW | PAGE_HUGE | phys_addr;
}

static uint64_t make_p2_entry_huge(const uint64_t phys_addr)
{
    return PAGE_PRESENT | PAGE_RW | PAGE_HUGE | phys_addr;
}

void init_identity_mapping()
{
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 512; j++) {
            p2_tables[i][j] = make_p2_entry_huge(j * (2 << 20));
        }
        p3_table[i] = make_p3_entry(reinterpret_cast<uint64_t>(p2_tables[i]));
    }
    p4_table[0] = make_p4_entry(reinterpret_cast<uint64_t>(p3_table));
    const uint64_t p4_addr = reinterpret_cast<uint64_t>(p4_table);
    asm volatile("mov %0, %%rax\n\tmov %%rax, %%cr3" : : "m" (p4_addr) : "memory", "eax");
}

} // namespace otrix::arch
