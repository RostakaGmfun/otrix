cmake_minimum_required(VERSION 3.0.2)

project(otrix C ASM)

set(TARGET_ARCH x86_64)
SET(CMAKE_SHARED_LIBRARY_LINK_C_FLAGS)

enable_testing()

set(KERNEL_C_FLAGS -ffreestanding -fPIC -O0)

add_subdirectory(common)
add_subdirectory(arch/${TARGET_ARCH})
add_subdirectory(dev)
add_subdirectory(kernel)
