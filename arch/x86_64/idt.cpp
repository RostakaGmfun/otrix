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
