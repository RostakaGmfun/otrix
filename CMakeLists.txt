cmake_minimum_required(VERSION 3.0.2)

project(otrix C ASM)

set(CMAKE_SHARED_LIBRARY_LINK_C_FLAGS)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -nostdlib -ffreestanding -nodefaultlibs -fPIC -O0")

add_library(otrix_x86_64 ${CMAKE_CURRENT_LIST_DIR}/arch/x86_64/multiboot.s
  ${CMAKE_CURRENT_LIST_DIR}/arch/x86_64/boot.s
  ${CMAKE_CURRENT_LIST_DIR}/arch/x86_64/context.s)

target_include_directories(otrix_x86_64 PUBLIC ${CMAKE_CURRENT_LIST_DIR}/arch/x86_64/include/
  ${CMAKE_CURRENT_LIST_DIR}/common/include)

add_library(otrix_dev ${CMAKE_CURRENT_LIST_DIR}/dev/serial.c ${CMAKE_CURRENT_LIST_DIR}/dev/pci.c)
target_include_directories(otrix_dev PUBLIC ${CMAKE_CURRENT_LIST_DIR}/dev/include)
target_link_libraries(otrix_dev otrix_x86_64)

add_executable(otrix_kernel kernel/kmain.c kernel/kthread.c)
target_link_libraries(otrix_kernel otrix_dev otrix_x86_64 -T${CMAKE_CURRENT_LIST_DIR}/arch/x86_64/linker.ld -n)
target_include_directories(otrix_kernel PUBLIC ${CMAKE_CURRENT_LIST_DIR}/kernel/include)

install(TARGETS otrix_kernel RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX}/bin)
