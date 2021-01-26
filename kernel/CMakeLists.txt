add_library(otrix_kmem kmem.c)
target_include_directories(otrix_kmem PUBLIC include/)

add_executable(kmem_test test/test_runner.c test/kmem_test.c)
target_include_directories(kmem_test PUBLIC include/)
target_link_libraries(kmem_test unity otrix_kmem)
add_test(NAME kmem_test COMMAND kmem_test)

add_executable(otrix_kernel kmain.cpp kthread.cpp)
target_link_libraries(otrix_kernel otrix_dev otrix_arch otrix_kmem -nodefaultlibs -nostdlib)
target_include_directories(otrix_kernel PUBLIC include/)
target_compile_options(otrix_kernel PUBLIC ${KERNEL_C_FLAGS} ${KERNEL_CXX_FLAGS})

install(TARGETS otrix_kernel RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX}/bin)
