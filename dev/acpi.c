#include "dev/acpi.hpp"
#include <stdint.h>
#include <stddef.h>

#define ACPI_BIOS_AREA_START ((void *)0x000E0000)
#define ACPI_BIOS_AREA_END ((void *)0x000FFFFF)

static const char rsdp_signature[] = "RSD PTR";
static const char rsdt_signature[] = "XSDT";
static const char madt_signature[] = "MADT";

// TODO(RostakaGmfun): move to common/
static size_t strlen(const char *str)
{
    size_t ret = 0;
    while (*str++) ret++;
    return ret;
}

static int memcmp(const void *s1, const void *s2, size_t size)
{
    const uint8_t *us1 = s1, *us2 = s2;
    while (size--) {
        int diff = *us1 - *us2;
        if (diff != 0) {
            return diff;
        }

        *us1++;
        *us2++;
    }

    return 0;
}

struct acpi_rsdp
{
    uint8_t signature[8]; // "RSD PTR "
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
    uint8_t signature[4];
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
    uint32_t local_apic_address;
    uint32_t flags;
    uint8_t entries[];
} __attribute__((packed));

enum acpi_madt_entry_type
{
    ACPI_MADT_LAPIC = 0,
    ACPI_MADT_IOAPIC = 1,
    ACPI_MADT_LAPIC_OVERRIDE = 2,
    ACPI_MADR_NMI = 4,
};

struct acpi_madt_entry_hdr
{
    uint8_t type;
    uint8_t length;
} __attribute__((packed));

struct acpi_madt_ioapic
{
    struct acpi_madt_entry_hdr hdr;
    uint8_t ioapic_id;
    uint8_t reserved;
    uint32_t ioapic_base;
    uint32_t global_system_interrupt_base;
} __attribute__((packed));

struct acpi_madt_lapic_override
{
    struct acpi_madt_entry_hdr hdr;
    uint16_t reserved;
    uint64_t lapic_address;
} __attribute__((packed));

static struct
{
    struct acpi_rsdp *rsdp;
    struct acpi_rsdt *rsdt;
    void *ioapic_base;
    uint8_t ioapic_id;
    void *lapic_base;
} acpi_context;

static error_t acpi_validate_checksum(const struct acpi_sdt_hdr *sdt)
{
    uint8_t checksum = 0;
    for (int i = 0; i < sdt->length; i++) {
        checksum += ((const uint8_t *)sdt)[i];
    }

    return (checksum == 0 ? EOK : EINVAL);
}

static void acpi_madt_parse_ioapic(const struct acpi_madt_ioapic *ioapic)
{
    acpi_context.ioapic_base = (void *)((uint64_t)ioapic->ioapic_base);
    acpi_context.ioapic_id = ioapic->ioapic_id;
}

static error_t acpi_parse_madt(const struct acpi_madt *madt)
{
    size_t num_entries = (madt->hdr.length - sizeof(struct acpi_madt))/8;
    const uint8_t *entry = madt->entries;

    acpi_context.lapic_base = (void *)(uintptr_t)madt->local_apic_address;

    while (num_entries > 0) {
        const struct acpi_madt_entry_hdr *hdr = (void *)entry;
        entry += madt->hdr.length;
        num_entries--;
        switch (hdr->type) {
        case ACPI_MADT_LAPIC_OVERRIDE:
            acpi_context.lapic_base = (void *)((struct acpi_madt_lapic_override *)hdr)->lapic_address;
            break;
        case ACPI_MADT_IOAPIC:
            acpi_madt_parse_ioapic((void *)hdr);
            break;
        default:
            break;
        }
    }

    return EOK;
}

static error_t acpi_parse_sdt_entry(uint64_t addr)
{
    const struct acpi_sdt_hdr *hdr = (const struct acpi_sdt_hdr *)addr;
    if (acpi_validate_checksum(hdr) != EOK) {
        return EINVAL;
    }

    if (memcmp(hdr->signature, madt_signature, strlen(madt_signature)) == 0) {
        return acpi_parse_madt((void *)hdr);
    }

    return EINVAL;
}

static error_t acpi_parse_rsdt(void)
{
    if (acpi_validate_checksum((void *)acpi_context.rsdt) != EOK) {
        return EINVAL;
    }

    if (memcmp(&acpi_context.rsdt->hdr.signature, rsdt_signature,
                    strlen(rsdt_signature)) != 0) {
        return EINVAL;
    }

    const int num_entries = (acpi_context.rsdt->hdr.length - sizeof(struct acpi_rsdt))/8;
    for (int i = 0; i < num_entries; i++) {
        error_t ret = acpi_parse_sdt_entry(acpi_context.rsdt->sdt_pointers[i]);
        if (ret != EOK) {
            return ret;
        }
    }

    return EOK;
}

error_t acpi_init(void)
{
    uint64_t *ptr = ACPI_BIOS_AREA_START;

    while (ptr != ACPI_BIOS_AREA_END) {
        if (memcmp(ptr, rsdp_signature, strlen(rsdp_signature)) != 0) {
            ptr++;
            continue;
        }
        acpi_context.rsdp = (struct acpi_rsdp *)ptr;
        if (acpi_context.rsdp->revision == 0) {
            return EINVAL; // not supported
        } else {
            acpi_context.rsdt = (void *)acpi_context.rsdp->xsdt_addr;
        }

        return acpi_parse_rsdt();
    }

    return ENODEV;
}

void *acpi_get_io_apic(void)
{
    return acpi_context.ioapic_base;
}

void *acpi_get_lapic_addr(void)
{
    return acpi_context.lapic_base;
}
