#ifndef OTRIX_ACPI_H
#define OTRIX_ACPI_H

#include "common/error.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

/**
 * Parse ACPI tables into internal storage
 * for further retrieval.
 */
error_t acpi_init(void);

void *acpi_get_ioapic_addr(void);

#ifdef __cplusplus
} // extern "C"
#endif // __cplusplus

#endif // OTRIX_ACPI_H
