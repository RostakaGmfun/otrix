add_executable(otrix_kernel kmain.cpp kthread.cpp)
target_link_libraries(otrix_kernel otrix_dev otrix_arch -nodefaultlibs -nostdlib)
target_include_directories(otrix_kernel PUBLIC include/)
target_compile_options(otrix_kernel PUBLIC ${KERNEL_C_FLAGS} ${KERNEL_CXX_FLAGS})

install(TARGETS otrix_kernel RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX}/bin)
