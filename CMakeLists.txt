cmake_minimum_required(VERSION 3.0.2)

project(otrix C ASM)

set(TARGET_ARCH x86_64)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -nostdlib -ffreestanding -nodefaultlibs -fPIC -O0")

add_subdirectory(common)
add_subdirectory(arch/${TARGET_ARCH})
add_subdirectory(dev)
add_subdirectory(kernel)
