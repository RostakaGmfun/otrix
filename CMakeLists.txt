cmake_minimum_required(VERSION 3.0.2 REQUIRED)

project(otrix ASM_NASM)

add_library(otrix_x86_64 ${CMAKE_CURRENT_LIST_DIR}/arch/x86_64/boot.asm
  ${CMAKE_CURRENT_LIST_DIR}/arch/x86_64/multiboot.asm)
target_link_libraries(otrix_x86_64 -T${CMAKE_CURRENT_LIST_DIR}/arch/x86_64/linker.ld)
target_include_directories(otrix_x86_64 ${CMAKE_CURRENT_LIST_DIR}/arch/x86_64/include/)

add_library(otrix_dev ${CMAKE_CURRENT_LIST_DIR}/dev/serial.c)
target_include_directories(otrix_dev PUBLIC ${CMAKE_CURRENT_LIST_DIR}/dev/)

add_executable(otrix_kernel core/kmain.c)
target_include_direcrories(otrix_kernel PUBLIC ${CMAKE_CURRENT_LIST_DIR}/core/)

target_link_libraries(otrix_kernel otrix_dev otrix_x86_64)
