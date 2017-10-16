#ifndef OTRIX_ARCH_PAGING_HPP
#define OTRIX_ARCH_PAGING_HPP

#include <cstddef>

namespace otrix::arch
{

//! Identity map the address space.
//!
//! \note This function will upate cr3 register.
void init_identity_mapping();

} // namespace otrix::arch

#endif // OTRIX_ARCH_PAGING_HPP
