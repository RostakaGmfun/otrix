cmake_minimum_required(VERSION 3.0.2)

project(otrix C CXX ASM)

set(TARGET_ARCH x86_64)

#add_compile_options($<$<COMPILE_LANGUAGE:CXX>:-fno-unwind-tables -std=c++17 -fno-rtti>)

option(BUILD_HOST_TESTS "Build host tests instead of kernel" OFF)

if(BUILD_HOST_TESTS)
  enable_testing()
else()
  add_subdirectory(arch/${TARGET_ARCH})
  add_subdirectory(dev)
endif()

add_subdirectory(common)
add_subdirectory(kernel)
