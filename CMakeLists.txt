cmake_minimum_required(VERSION 3.0.2)

project(otrix C CXX ASM)

set(TARGET_ARCH x86_64)
SET(CMAKE_SHARED_LIBRARY_LINK_C_FLAGS)

enable_testing()

set(KERNEL_C_FLAGS -ffreestanding -fPIC -O0)
set(KERNEL_CXX_FLAGS -fno-exceptions -fno-unwind-tables
  -nostdlib -fno-rtti -std=c++17 -static)

add_subdirectory(common)
add_subdirectory(arch/${TARGET_ARCH})
add_subdirectory(dev)
add_subdirectory(kernel)
