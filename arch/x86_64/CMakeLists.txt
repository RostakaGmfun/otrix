add_library(otrix_arch boot.s context.s irq_manager.cpp paging.cpp pic.cpp interrupts.s lapic.cpp kvmclock.cpp)
target_include_directories(otrix_arch PUBLIC include/ ../../api)
target_link_libraries(otrix_arch otrix_common otrix_kernel -T${CMAKE_CURRENT_LIST_DIR}/linker.ld -n -nostartfiles -ggdb3)
target_compile_options(otrix_arch PUBLIC ${KERNEL_C_FLAGS})
