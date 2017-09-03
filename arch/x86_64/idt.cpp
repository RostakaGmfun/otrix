#include "arch/idt.h"
#include <cstdint>

struct idt
{
    uint16_t offset_low; /**< Bits 0-15 */
    uint16_t cs;
    uint8_t ist; /**< Interrupt stack table */
    uint8_t type_dpl; /**< Folks say should be 0x8E */
    uint16_t offset_mid; /**< Bits 16-31 */
    uint32_t offset_high; /**< Bits 32-63 */
} __attribute__((packed));

namespace __idt_detail
{

constexpr auto NUM_IDT_ENTRIES = 31;
static idt idt_table[NUM_IDT_ENTRIES] = { 0 };

constexpr auto generate_idt()
{
    return nullptr;
}

} // namespace __idt_detail

constexpr idt *idt_table_ptr = __idt_detail::generate_idt();
