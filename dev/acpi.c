#include "arch/acpi.h"
#include <stdint.h>

#define ACPI_BIOS_AREA_START 0x000E0000
#define ACPI_BIOS_AREA_END 0x000FFFFF

struct acpi_rsdp
{
    uint64_t signature; // "RSD PTR "
    uint8_t checksum;
    char OEMID[6];
    uint8_t revision;
    uint32_t rsdt_addr;

    // Version 2.0 fields
    uint32_t length;
    uint64_t xsdt_addr;
    uint8_t extended_checksum;
    uint8_t reserved[3];
} __attribute__((packed));

struct acpi_sdt_hdr
{
    uint32_t signature;
    uint32_t length;
    uint8_t revision;
    uint8_t checksum;
    uint8_t OEMID[6];
    uint8_t OEMTableID[8];
    uint32_t OEMRevision;
    uint32_t creator_id;
    uint32_t creator_revision;

} __attribute__((packed));

/**
 * Signature is either 'RSDT' or 'XSDT'
 */
struct acpi_rsdt
{
    struct acpi_sdt_hdr hdr;
    uint8_t sdt_pointers[];
} __attribute__((packed));

/**
 * Signature is 'APIC'
 */
struct acpi_madt
{
    struct acpi_sdt_hdr hdr;
    uint32_t local_controller_addr;
    uint32_t flags;
    uint8_t entries[];
} __attribute__((packed));

enum acpi_madt_entry
{
    ACPI_MADT_LAPIC = 0,
    ACPI_MADT_IOAPIC = 1,
    ACPI_MADT_INTR_OVERRIDE = 2,
    ACPI_MADR_NMI = 4,
};

static struct
{
    struct acpi_rsdp *rsdp;
    struct acpi_rsdt *rsdt;
    struct acpi_madr *madt;
} acpi_context;

static error_code_t acpi_validate_checksum(const struct acpi_sdt_hdr *sdt)
{
    uint8_t checksum = 0;
    for (int i = 0; i < sdt->length; i++) {
        checksum += ((const uint8_t *)sdt)[i];
    }

    return (checksum == 0 ? EOK : EINVAL);
}

static error_code_t acpi_parse_sdt(uint64_t addr)
{
    const struct acpi_sdt_hdr *hdr = (const struct acpi_sdt_hdr *)addr;

}

static error_code_t acpi_parse_rsdt(void)
{
    if (acpi_validate_checksum(acpi_context.rsdt) != EOK) {
        return EINVAL;
    }

    if (acpi_context.rsdt->signature != 'XSDT') {
        return EINVAL;
    }

    const num_entries = (acpi_context.rsdt->length - sizeof(struct acpi_rsdt))/8;
    for (int i = 0; i < num_entries; i++) {
        error_code_t ret = acpi_parse_sdt_entry(acpi_context.rsdt.sdt_pointers[i]);
        if (ret != EOK) {
            return ret;
        }
    }

    return EOK;
}

error_code_t acpi_init(void)
{
    uint64_t *ptr = ACPI_BIOS_AREA_START;

    while (ptr != ACPI_BIOS_AREA_END) {
        if (*ptr == 'RSD PTR ') {
            acpi_context.rsdp = (struct acpi_rsdp *)ptr;
            if (acpi_context.rsdp.revision == 0) {
                acpi_context.rsdt = acpi_context.rsdp.rsdt_addr;
            } else {
                acpi_context.rsdt = acpi_context.rsdp.xsdt_addr;
            }

            return acpi_parse_rsdt();
        }
        ptr++;
    }

    return ENODEV;
}
