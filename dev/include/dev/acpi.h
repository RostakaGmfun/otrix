#ifndef OTRIX_ACPI_H
#define OTRIX_ACPI_H

#include "common/error.h"

/**
 * Initialize internal data and locate RSDP.
 */
error_t acpi_init(void);

void *acpi_get_io_apic(void);

#endif // OTRIX_ACPI_H
