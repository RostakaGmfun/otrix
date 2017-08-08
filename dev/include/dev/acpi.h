#ifndef OTRIX_ACPI_H
#define OTRIX_ACPI_H

#include "common/error.h"

/**
 * Initialize internal data and locate RSDP.
 */
error_code_t acpi_init(void);

error_code_t acpi_get_io_apic(uint64_t *const apic_addr);

#endif // OTRIX_ACPI_H
