add_library(otrix_arch boot.s context.s idt.cpp paging.cpp)
target_include_directories(otrix_arch PUBLIC include/ ../../api)
target_link_libraries(otrix_arch otrix_common -T${CMAKE_CURRENT_LIST_DIR}/linker.ld -n -nostartfiles)
target_compile_options(otrix_arch PUBLIC ${KERNEL_C_FLAGS})
