add_library(otrix_arch boot.s context.s)
target_include_directories(otrix_arch PUBLIC include/)
target_link_libraries(otrix_arch  -T${CMAKE_CURRENT_LIST_DIR}/linker.ld -n)
