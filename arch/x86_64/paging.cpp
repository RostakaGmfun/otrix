#include "arch/paging.hpp"

#include <cstdint>
#include <type_traits>

namespace otrix::arch
{

static constexpr auto page_table_alignment = 4096;

static constexpr auto p4_size = 512;
static constexpr auto p3_size = 512;
static constexpr auto p2_size = 512;
static constexpr auto p2_num_tables = 4;

static typename std::aligned_storage<sizeof(uint64_t) * p4_size,
                              page_table_alignment>::type p4_data;

static typename std::aligned_storage<sizeof(uint64_t) * p3_size,
                              page_table_alignment>::type p3_data;

static typename std::aligned_storage<sizeof(uint64_t) * p2_size,
                              page_table_alignment>::type p2_data;

static uint64_t *p4_table = reinterpret_cast<uint64_t *>(&p4_data);
static uint64_t *p3_table = reinterpret_cast<uint64_t *>(&p3_data);

// TODO(RostakaGmfun): Make the p2_tables depend on p2_num_tables
static uint64_t *p2_tables[] = {
    reinterpret_cast<uint64_t *>(&p2_data),
    reinterpret_cast<uint64_t *>(&p2_data) + p2_size * sizeof(uint64_t),
    reinterpret_cast<uint64_t *>(&p2_data) + 2 * p2_size * sizeof(uint64_t),
    reinterpret_cast<uint64_t *>(&p2_data) + 3 * p2_size * sizeof(uint64_t),
};

constexpr auto PAGE_PRESENT = 1LU << 0;
constexpr auto PAGE_RW = 1LU << 1;

static uint64_t make_p4_entry(const uint64_t p3_addr)
{
    return PAGE_PRESENT | PAGE_RW | p3_addr;
}

static uint64_t make_p3_entry(const uint64_t p2_addr)
{
    return PAGE_PRESENT | PAGE_RW | p2_addr;
}

static uint64_t make_p2_entry(const uint64_t phys_addr)
{
    return PAGE_PRESENT | PAGE_RW | (1 << 7) | phys_addr;
}

//! Map 1G of physical address space into P2 table with 2M pages
static void map_p2_table(uint64_t *p2_table, uint64_t phys_addr)
{
    for (int i = 0; i < p2_size; i++) {
        p2_table[i] = make_p2_entry(phys_addr);
        phys_addr += (2 << 20); // 2M
    }
}

void init_identity_mapping()
{
    const size_t size_512M = (512 << 20);
    for (int i = 0; i < p2_num_tables; i++) {
        map_p2_table(p2_tables[i], i * size_512M);
        p3_table[i] = make_p3_entry(reinterpret_cast<uint64_t>(p2_tables[i]));
    }
    p4_table[0] = make_p4_entry(reinterpret_cast<uint64_t>(p3_table));
    const uint64_t p4_addr = reinterpret_cast<uint64_t>(p4_table);
    __asm__ volatile("mov %0, %%rax\n\tmov %%rax, %%cr3" : : "m" (p4_addr) : "memory", "eax");
}

} // namespace otrix::arch
