#pragma once

#include "common/error.h"
#include <stdint.h>

/**
 * Parse ACPI tables into internal storage
 * for further retrieval.
 */
error_t acpi_init(void);

void *acpi_get_ioapic_addr(void);

void *acpi_get_lapic_addr(void);
